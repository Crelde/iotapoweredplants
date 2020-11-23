#include "XDKAppInfo.h"

#undef BCDS_MODULE_ID  /* Module ID define before including Basics package*/
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_HTTP_IOT2TANGLE_CLIENT

/* own header files */
#include "IotaPlants.h"

/* system header files */
#include <stdio.h>

/* additional interface header files */

#include "XDK_WLAN.h"
#include "XDK_ServalPAL.h"
#include "XDK_HTTPRestClient.h"
#include "XDK_SNTP.h"
#include "BCDS_BSP_Board.h"
#include "BCDS_NetworkConfig.h"
#include "BCDS_CmdProcessor.h"
#include "BCDS_Assert.h"
#include "XDK_Utils.h"
#include "FreeRTOS.h"
#include "task.h"

#include <Serval_Clock.h>
#include <Serval_Log.h>
#include "FreeRTOS.h"
#include "timers.h"
#include "Serval_Http.h"
#include "EnvironmentalSensor.h"
#include "Accelerometer.h"
#include "Gyroscope.h"
#include "InertialSensor.h"
#include "LightSensor.h"
#include "Magnetometer.h"
#include "semphr.h"
#include "External.h"


static xTaskHandle httpGetTaskHandle;
static xTaskHandle httpPostTaskHandle;
static xTimerHandle triggerHttpRequestTimerHandle;
static uint32_t httpGetPageOffset = 0;
CmdProcessor_T *AppCmdProcessorHandle;
static uint32_t SysTime = UINT32_C(0);
HTTPRestClient_Config_T HTTPRestClientConfigInfo;
static SemaphoreHandle_t semPost = NULL;

// Global array of all sensors => true : enable -- false : disable
bool typesSensors[8] = {
						true, //ENVIRONMENTAL
						true, //ACCELEROMETER
						true, //GYROSCOPE
						true, //INERTIAL
						true, //LIGHT
						true, //MAGNETOMETER
						true,  //ACOUSTIC
						true, //EXTERNAL
					};


static retcode_t httpRequestSentCallback(Callable_T* caller, retcode_t callerStatus)
{
    BCDS_UNUSED(caller);

    if (RC_OK == callerStatus)
    {
        printf("httpRequestSentCallback: HTTP request sent successfully.\r\n");
    }
    else
    {
        printf("httpRequestSentCallback: HTTP request failed to send. error=%d\r\n", callerStatus);
        printf("httpRequestSentCallback: Restarting request timer\r\n");
        xTimerStart(triggerHttpRequestTimerHandle, 10);
    }

    return RC_OK;
}

static retcode_t httpGetResponseCallback(HttpSession_T *httpSession, Msg_T *httpMessage, retcode_t status)
{
    BCDS_UNUSED(httpSession);

    xTimerStart(triggerHttpRequestTimerHandle, INTER_REQUEST_INTERVAL / portTICK_PERIOD_MS);

    if (RC_OK != status)
    {
        printf("httpGetResponseCallback: error while receiving response to GET request. error=%d\r\n", status);
        return RC_OK;
    }
    if (NULL == httpMessage)
    {
        printf("httpGetResponseCallback: received NULL as HTTP message. This should not happen.\r\n");
        return RC_OK;
    }

    Http_StatusCode_T httpStatusCode = HttpMsg_getStatusCode(httpMessage);
    if (Http_StatusCode_OK != httpStatusCode)
    {
        printf("httpGetResponseCallback: received HTTP status other than 200 OK. status=%d\r\n", httpStatusCode);
    }
    else
    {
        retcode_t retcode;
        bool isLastPartOfMessage;
        uint32_t pageContentSize;
        retcode = HttpMsg_getRange(httpMessage, UINT32_C(0), &pageContentSize, &isLastPartOfMessage);
        if (RC_OK != retcode)
        {
            printf("httpGetResponseCallback: failed to get range from message. error=%d\r\n", retcode);
        }
        else
        {
            const char* responseContent;
            unsigned int responseContentLen;
            HttpMsg_getContent(httpMessage, &responseContent, &responseContentLen);
            printf("httpGetResponseCallback: successfully received a response: %.*s\r\n", responseContentLen, responseContent);

            if (isLastPartOfMessage)
            {
                /* We're done with the GET request. Let's make a POST request. */
                printf("httpGetResponseCallback: Server is up. Triggering the POST request.\r\n");
                xTaskNotifyGive(httpPostTaskHandle);
            }
            else
            {
                /* We're not done yet downloading the page - let's make another request. */
                printf("httpGetResponseCallback: there is still more to GET. Making another request.\r\n");
                httpGetPageOffset += responseContentLen;
                xTaskNotifyGive(httpGetTaskHandle);
            }
        }
    }
    return RC_OK;
}

static void httpGetTask(void* parameter)
{
    BCDS_UNUSED(parameter);
    retcode_t retcode;
    Retcode_T retVal;
    Msg_T* httpMessage;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);


        Ip_Address_T destServerAddress;
        retVal = NetworkConfig_GetIpAddress((uint8_t*) DEST_SERVER_HOST, &destServerAddress);
        if (RETCODE_OK != retVal)
        {
            printf("httpGetTask: unable to resolve hostname " DEST_SERVER_HOST ". error=%d.\r\n", retcode);
        }
        if (RETCODE_OK == retVal)
        {

            retcode = HttpClient_initRequest(&destServerAddress, Ip_convertIntToPort(DEST_SERVER_PORT), &httpMessage);

            if (RC_OK != retcode)
            {
                printf("httpGetTask: unable to create HTTP request. error=%d.\r\n", retcode);
                retVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_INIT_REQUEST_FAILED);
            }
        }
        if (RETCODE_OK == retVal)
        {
            HttpMsg_setReqMethod(httpMessage, Http_Method_Get);

            HttpMsg_setContentType(httpMessage, Http_ContentType_Text_Plain);

            retcode = HttpMsg_setReqUrl(httpMessage, DEST_GET_PATH);
            if (RC_OK != retcode)
            {
                printf("httpGetTask: unable to set request URL. error=%d.\r\n", retcode);
                retVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_SET_REQURL_FAILED);
            }
        }

        if (RETCODE_OK == retVal)
        {
            retcode = HttpMsg_setHost(httpMessage, DEST_SERVER_HOST);
            if (RC_OK != retcode)
            {
                printf("httpGetTask: unable to set HOST header. error=%d.\r\n", retcode);
                retVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_SET_HOST_FAILED);
            }
        }

        if (RETCODE_OK == retVal)
        {
            HttpMsg_setRange(httpMessage, httpGetPageOffset, REQUEST_MAX_DOWNLOAD_SIZE);

            Callable_T httpRequestSentCallable;
            (void) Callable_assign(&httpRequestSentCallable, httpRequestSentCallback);
            retcode = HttpClient_pushRequest(httpMessage, &httpRequestSentCallable, httpGetResponseCallback);
            if (RC_OK != retcode)
            {
                printf("httpGetTask: unable to push the HTTP request. error=%d.\r\n", retcode);
                retVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_PUSH_REQUEST_FAILED);
            }
        }
        if (RETCODE_OK != retVal)
        {
            Retcode_RaiseError(retVal);
            xTimerStart(triggerHttpRequestTimerHandle, INTER_REQUEST_INTERVAL / portTICK_PERIOD_MS);
        }
    }
}


static retcode_t httpPostCustomHeaderSerializer(OutMsgSerializationHandover_T* serializationHandover)
{
    if (serializationHandover == NULL)
    {
        printf("httpPostCustomHeaderSerializer: serializationHandover is NULL. This should never happen.\r\n");
        return RC_APP_ERROR;
    }

    retcode_t result = RC_OK;
    switch (serializationHandover->position)
    {
    case 0:
        result = TcpMsg_copyStaticContent(serializationHandover, POST_REQUEST_CUSTOM_HEADER_0, strlen(POST_REQUEST_CUSTOM_HEADER_0));
        if (result != RC_OK)
            return result;
        serializationHandover->position = 1;
        break;
    case 1:
        result = TcpMsg_copyContentAtomic(serializationHandover, POST_REQUEST_CUSTOM_HEADER_1, strlen(POST_REQUEST_CUSTOM_HEADER_1));
        if (result != RC_OK)
            return result;
        serializationHandover->position = 2;
        break;
    default:
        result = RC_OK;
    }
    return result;

}

uint32_t GetUtcTime() {
  retcode_t rc = RC_CLOCK_ERROR_FATAL;
  uint32_t sysUpTime;
  rc = Clock_getTime(&sysUpTime);
  if (rc != RC_OK) {
    printf("Failed to get the Clock Time \r\n");
  }
  return sysUpTime + SysTime;
}


static char* receiveBufferFromSensors(void){
	int i=0;
	bool typeSensor;

    char  *buffer = calloc(1024, sizeof(char));
    char *aux;


	strcat(buffer,"{\"iot2tangle\":[");

	for(i=0;i<MAX_SENSORS_ARRAY;i++){

		typeSensor = typesSensors[i];

		if(typeSensor){

			switch(i)
		    {
				case ENVIRONMENTAL:
					aux = processEnvSensorData(null,0);
					break;
				case ACCELEROMETER:
					aux = processAccelData(null,0);
					break;
				case GYROSCOPE:
					aux = processGyroData(null,0);
					break;
				case INERTIAL:
					aux = processInertiaSensor(null,0);
					break;
				case LIGHT:
					aux = processLightSensorData(null,0);
					break;
				case MAGNETOMETER:
					aux = processMagnetometerData(null,0);
					break;
				case ACOUSTIC:
					aux = processAcousticData(null,0);
					break;
				case EXTERNAL:
					aux = processExternalData(null,0);
					break;
		    }
			strcat(buffer,aux);
			strcat(buffer,",");
			free(aux);
		}
	}

	if(buffer[strlen(buffer)-1]==',')
		buffer[strlen(buffer)-1]=' ';

	char * deviceName = calloc(255, sizeof(char));
	char * timestamp = calloc(255, sizeof(char));
	sprintf(deviceName,"\"device\": \"%s\",",DEVICE_NAME);
	sprintf(timestamp,"\"timestamp\": \"%ld\"}",GetUtcTime());
	strcat(buffer,"],");
	strcat(buffer,deviceName);
	strcat(buffer,timestamp);
	free(deviceName);
	free(timestamp);

	return (char*)buffer;

}



static retcode_t httpPostPayloadSerializer(OutMsgSerializationHandover_T* serializationHandover)
{
    char* httpBodyBuffer = receiveBufferFromSensors();

    uint32_t offset = serializationHandover->offset;
    uint32_t bytesLeft = strlen(httpBodyBuffer) - offset;
    uint32_t bytesToCopy = serializationHandover->bufLen > bytesLeft ? bytesLeft : serializationHandover->bufLen;

    memcpy(serializationHandover->buf_ptr, httpBodyBuffer + offset, bytesToCopy);
    serializationHandover->len = bytesToCopy;
    free(httpBodyBuffer);


    if (bytesToCopy < bytesLeft)
    {
        return RC_MSG_FACTORY_INCOMPLETE;
    }
    else
    {
        return RC_OK;
    }
}

static retcode_t httpPostResponseCallback(HttpSession_T *httpSession, Msg_T *httpMessage, retcode_t status)
{
    BCDS_UNUSED(httpSession);


    if (RC_OK != status)
    {
        return RC_APP_ERROR;
    }
    if (NULL == httpMessage)
    {
        printf("httpPostResponseCallback: received NULL as HTTP message. This should not happen.\r\n");
        return RC_APP_ERROR;
    }

    Http_StatusCode_T httpStatusCode = HttpMsg_getStatusCode(httpMessage);
    if (Http_StatusCode_OK != httpStatusCode)
    {
        printf("httpPostResponseCallback: received HTTP status other than 200 OK. status=%d\r\n", httpStatusCode);
    }
    else
    {
        retcode_t retcode;
        bool isLastPartOfMessage;
        uint32_t pageContentSize;
        retcode = HttpMsg_getRange(httpMessage, UINT32_C(0), &pageContentSize, &isLastPartOfMessage);
        if (RC_OK != retcode)
        {
            printf("httpPostResponseCallback: failed to get range from message. error=%d\r\n", retcode);
        }
        else
        {
            const char* responseContent;
            unsigned int responseContentLen;
            HttpMsg_getContent(httpMessage, &responseContent, &responseContentLen);
            printf("httpPostResponseCallback: successfully received a response: %.*s\r\n", responseContentLen, responseContent);

            if (!isLastPartOfMessage)
            {
                printf("httpPostResponseCallback: server response was too large. This example application does not support POST responses larger than %lu.\r\n", REQUEST_MAX_DOWNLOAD_SIZE);
            }

            printf("httpPostResponseCallback: POST request is done. Restarting request timer.\r\n");

        }
    }

    return RC_OK;
}



static void httpPostTask(void* parameter)
{
    BCDS_UNUSED(parameter);
    retcode_t retcode;
    Retcode_T retVal;
    Msg_T* httpMessage;
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

   while (1)
    {
	   	xSemaphoreTake(semPost,( TickType_t ) INTER_REQUEST_INTERVAL);

        Ip_Address_T destServerAddress;
        retVal = NetworkConfig_GetIpAddress((uint8_t*) DEST_SERVER_HOST, &destServerAddress);
        if (RETCODE_OK != retVal)
        {
            printf("httpPostTask: unable to resolve hostname " DEST_SERVER_HOST ". error=%d.\r\n", (int) retVal);
        }
        if (RETCODE_OK == retVal)
        {
            retcode = HttpClient_initRequest(&destServerAddress, Ip_convertIntToPort(DEST_SERVER_PORT), &httpMessage);

            if (RC_OK != retcode)
            {
                printf("httpPostTask: unable to create HTTP request. error=%d.\r\n", retcode);
                retVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_INIT_REQUEST_FAILED);
            }
        }
        if (RETCODE_OK == retVal)
        {
            HttpMsg_setReqMethod(httpMessage, Http_Method_Post);

            HttpMsg_setContentType(httpMessage, Http_ContentType_App_Json);

            retcode = HttpMsg_setReqUrl(httpMessage, DEST_POST_PATH);
            if (RC_OK != retcode)
            {
                printf("httpPostTask: unable to set request URL. error=%d.\r\n", retcode);
                retVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_SET_REQURL_FAILED);
            }
        }
        if (RETCODE_OK == retVal)
        {
            retcode = HttpMsg_setHost(httpMessage, DEST_SERVER_HOST);
            if (RC_OK != retcode)
            {
                printf("httpPostTask: unable to set HOST header. error=%d.\r\n", retcode);
                retVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_SET_HOST_FAILED);
            }
        }
        if (RETCODE_OK == retVal)
        {
            HttpMsg_setRange(httpMessage, httpGetPageOffset, REQUEST_MAX_DOWNLOAD_SIZE);

            HttpMsg_serializeCustomHeaders(httpMessage, httpPostCustomHeaderSerializer);

            retcode = TcpMsg_prependPartFactory(httpMessage, httpPostPayloadSerializer);
            if (RC_OK != retcode)
            {
                printf("httpPostTask: unable to serialize request body. error=%d.\r\n", retcode);
                retVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_FAILURE);
            }
        }
        if (RETCODE_OK == retVal)
        {
            Callable_T httpRequestSentCallable;
            (void) Callable_assign(&httpRequestSentCallable, httpRequestSentCallback);
            retcode = HttpClient_pushRequest(httpMessage, &httpRequestSentCallable, httpPostResponseCallback);
            if (RC_OK != retcode)
            {
                printf("httpPostTask: unable to push the HTTP request. error=%d.\r\n", retcode);
                retVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_PUSH_REQUEST_FAILED);
            }
        }

        if (RETCODE_OK != retVal)
        {
            Retcode_RaiseError(retVal);
        }
    }
}

static void triggerHttpRequestTimerCallback(TimerHandle_t timer)
{
    BCDS_UNUSED(timer);
    httpGetPageOffset = 0;

    //todo

}
void SetUtcTime(uint32_t utcTime) {
  retcode_t rc = RC_CLOCK_ERROR_FATAL;
  uint32_t sysUpTime;
  rc = Clock_getTime(&sysUpTime);
  if (rc != RC_OK) {
    printf("Failed to get the Clock Time \r\n");
  }
  SysTime = utcTime - sysUpTime;
}

static void ReceiveCallback(Msg_T *msg_ptr, retcode_t status) {


  unsigned int payloadLen;
  uint8_t *payload_ptr;

  if (status != RC_OK) {

  }
  XUdp_getXUdpPayload(msg_ptr, &payload_ptr, &payloadLen);



  if (payloadLen >= NTP_PACKET_SIZE) {
    uint64_t secsSince1900;
    /* convert 4 bytes starting at location 40 to a long integer */
    secsSince1900 = (unsigned long)payload_ptr[40] << 24;
    secsSince1900 |= (unsigned long)payload_ptr[41] << 16;
    secsSince1900 |= (unsigned long)payload_ptr[42] << 8;
    secsSince1900 |= (unsigned long)payload_ptr[43];
    /* subtract 70 years 2208988800UL; (Unix: starting from 1970) */
    uint64_t secsSince1970 = secsSince1900 - 2208988800UL; /* UTC else + timeZone*SECS_PER_HOUR; */
    printf("NTP got UTC secSince1970: %llu\n\r", secsSince1970);
    SetUtcTime(secsSince1970);
  } else {
    printf("NTP response not valid!\n\r");
  }
}

static void SendCallback(Msg_T *msg_ptr, retcode_t status) {
  BCDS_UNUSED(msg_ptr);

  printf("NTP request Sending Complete\n\r");

  if (status != RC_OK) {
    printf("Sending status not RC_OK; status=" RC_RESOLVE_FORMAT_STR "\n\r",
           RC_RESOLVE((int)status));
  }
}


void InitSntpTime() {
  uint32_t now = 0;
  retcode_t rc = RC_OK;
  Msg_T *MsgHandlePtr = NULL;
  Ip_Port_T port = Ip_convertIntToPort(
      SNTP_DEFAULT_PORT); // also used for local ntp client! */

  rc = XUdp_initialize();
  if (rc != RC_OK) {
	 printf("FAILED TO INIT\n");
    LOG_ERROR("Failed to init XUDP; rc=" RC_RESOLVE_FORMAT_STR "\n",RC_RESOLVE((int)rc));
    return;
  }

  rc = XUdp_start(port, ReceiveCallback);
  if (rc != RC_OK) {
	  printf("FAILED TO START\n");
    LOG_ERROR("Failed to start XUDP; rc=" RC_RESOLVE_FORMAT_STR "\n",RC_RESOLVE((int)rc));
    return;
  }

  Ip_Address_T sntpIpAddress;
  uint8_t buffer[NTP_PACKET_SIZE];
  unsigned int bufferLen = NTP_PACKET_SIZE;

  /* init request: */
  memset(buffer, 0, NTP_PACKET_SIZE);
  buffer[0] = 0b11100011; /* LI, Version, Mode */ /*lint !e146 */
  buffer[1] = 0;    /* Stratum, or type of clock */
  buffer[2] = 6;    /* Polling Interval */
  buffer[3] = 0xEC; /* Peer Clock Precision */
  /* 8 bytes of zero for Root Delay & Root Dispersion */
  buffer[12] = 49;
  buffer[13] = 0x4E;
  buffer[14] = 49;
  buffer[15] = 52;

  rc = Clock_getTime(&now);
  if (RC_OK == rc) {
    /* time available, use timeout */
    const uint32_t dnsEndTime = now + NTP_DNS_TIMEOUT_IN_S;
    while (dnsEndTime > now) {
      rc = NetworkConfig_GetIpAddress((uint8_t *)SNTP_DEFAULT_ADDR,
                            (Ip_Address_T *)&sntpIpAddress);
      if (RC_OK == rc)
        break;
      vTaskDelay(NTP_DNS_RETRY_INTERVAL_IN_MS / portTICK_RATE_MS);
      if (RC_OK != Clock_getTime(&now))
        break;
    }
  } else {
    /* no time, no retry */
    rc = NetworkConfig_GetIpAddress((uint8_t *)SNTP_DEFAULT_ADDR,
                          (Ip_Address_T *)&sntpIpAddress);
  }

  /* resolve IP address of server hostname: 0.de.pool.ntp.org eg.: 176.9.104.147
   */
  if (RC_OK != rc) {
    printf("NTP Server %s could not be resolved. Use a static IP "
           "(129.6.15.28) instead!\n\r",
           SNTP_DEFAULT_ADDR);
    /* dirctly use a "0.de.pool.ntp.org" pool IP: 176.9.104.147 */
    Ip_convertOctetsToAddr(129, 6, 15, 28, &sntpIpAddress);
  }

  /* now send request: */
  rc = XUdp_push(&sntpIpAddress, SNTP_DEFAULT_PORT, buffer,
                 bufferLen, // to default NTP server port */
                 SendCallback, &MsgHandlePtr);
  if (rc != RC_OK) {
    LOG_ERROR("Sending failure; rc=" RC_RESOLVE_FORMAT_STR "\n",
              RC_RESOLVE((int)rc));
    return;
  }
  LOG_INFO("Pushed Echo\n");
  return;
}


void appInitSystem(void* cmdProcessorHandle, uint32_t param2)
{

	semPost = xSemaphoreCreateBinary();

	WLAN_Setup_T WLANSetupInfo =
				{
						.IsEnterprise = false,
						.IsHostPgmEnabled = false,
						.SSID = WLAN_SSID,
						.Username = WLAN_PSK,
						.Password = WLAN_PSK,
						.IsStatic = 0,
						.IpAddr = XDK_NETWORK_IPV4(0, 0, 0, 0),
						.GwAddr = XDK_NETWORK_IPV4(0, 0, 0, 0),
						.DnsAddr = XDK_NETWORK_IPV4(0, 0, 0, 0),
						.Mask = XDK_NETWORK_IPV4(0, 0, 0, 0),
				};

	HTTPRestClient_Setup_T HTTPRestClientSetupInfo =
	{
			.IsSecure = HTTP_SECURE_ENABLE,
	};

	HTTPRestClientConfigInfo.IsSecure = HTTP_SECURE_ENABLE;
	HTTPRestClientConfigInfo.DestinationServerUrl = DEST_SERVER_HOST;
	HTTPRestClientConfigInfo.DestinationServerPort = atoi(DEST_SERVER_PORT);
	HTTPRestClientConfigInfo.RequestMaxDownloadSize = REQUEST_MAX_DOWNLOAD_SIZE;

	BCDS_UNUSED(param2);
	Retcode_T returnValue = RETCODE_OK;
	AppCmdProcessorHandle = (CmdProcessor_T *) cmdProcessorHandle;

	retcode_t rc = RC_OK;
	rc = WLAN_Setup(&WLANSetupInfo);
	if (RC_OK != rc)
	{
		printf("appInitSystem: network init/connection failed. error=%d\r\n", rc);
		return;
	}
	printf("ServalPal Setup\r\n");
	returnValue = ServalPAL_Setup(AppCmdProcessorHandle);

	if (RETCODE_OK == returnValue)
	{
		returnValue = HTTPRestClient_Setup(&HTTPRestClientSetupInfo);
	}

	if (RETCODE_OK != returnValue)
	{
		Retcode_RaiseError(returnValue);
		printf("ServalPal Setup failed with %d \r\n", (int) returnValue);
		return;
	}

	Retcode_T retcode = WLAN_Enable();
	if (RETCODE_OK == retcode)
	{
		retcode = ServalPAL_Enable();
	}
	if (RETCODE_OK == retcode)
	{
	   retcode = HTTPRestClient_Enable();
	}
	if (RETCODE_OK != retcode){
		Retcode_RaiseError(retcode);
		printf("WLAN_Enable failed with %d \r\n", (int) retcode);
		return;
	}
	printf("Connecting to %s \r\n ", WLAN_SSID);
	rc = HTTPRestClient_Setup(&HTTPRestClientSetupInfo);
	if (RETCODE_OK != rc){
		Retcode_RaiseError(rc);
		printf("HTTPRestClient_Setup failed with %d \r\n", (int) rc);
		return;
	}
	rc = HTTPRestClient_Enable();
	if (RETCODE_OK != rc){
		Retcode_RaiseError(rc);
		printf("HTTPRestClient_Enable failed with %d \r\n", (int) rc);
		return;
	}

    InitSntpTime();

    int i=0;
	for(i=0;i<MAX_SENSORS_ARRAY;i++){
		if(typesSensors[i]){
			switch(i)
		    {
				case ENVIRONMENTAL:
					environmentalSensorInit();
					break;
				case ACCELEROMETER:
					accelerometerSensorInit();
					break;
				case GYROSCOPE:
					gyroscopeSensorInit();
					break;
				case INERTIAL:
					inertialSensorInit();
					break;
				case LIGHT:
					lightsensorInit();
					break;
				case MAGNETOMETER:
					magnetometerSensorInit();
					break;
				case ACOUSTIC:
					acousticSensorInit();
					break;
				case EXTERNAL:
					externalSensorInit();
					break;
		    }
		}
	}

	BaseType_t taskCreated;

	taskCreated = xTaskCreate(httpPostTask, "IOT2TANGLE Post", TASK_STACK_SIZE_HTTP_REQ, NULL, TASK_PRIO_HTTP_REQ, &httpPostTaskHandle);
	if (taskCreated != pdTRUE)
	{
		printf("Failed to create the POST request task\r\n");
		return;
	}

	triggerHttpRequestTimerHandle = xTimerCreate("triggerIOT2TANGLERequestTimer", INTER_REQUEST_INTERVAL / portTICK_RATE_MS, pdFALSE, NULL, triggerHttpRequestTimerCallback);
	if (triggerHttpRequestTimerHandle == NULL)
	{
		printf("Failed to create the triggerRequestTimer\r\n");
		return;
	}
	BaseType_t requestTimerStarted = xTimerStart(triggerHttpRequestTimerHandle, INTER_REQUEST_INTERVAL / portTICK_RATE_MS);
	if (requestTimerStarted == pdFALSE)
	{
		printf("Failed to start the triggerRequestTimer\r\n");
	}

	printf("Connected to network. First HTTP request will be made soon.\r\n");
	xTaskNotifyGive(httpPostTaskHandle);


}

CmdProcessor_T * GetAppCmdProcessorHandle(void)
{
    return AppCmdProcessorHandle;
}


