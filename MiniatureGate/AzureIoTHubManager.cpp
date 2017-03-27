#include "AzureIoTHubManager.h"
#include <Arduino.h>

using namespace std;

WiFiClientSecure AzureIoTHubManager::_sslWiFiClient;
AzureIoTHubClient AzureIoTHubManager::_iotHubClient;

AzureIoTHubManager::AzureIoTHubManager(WiFiManagerPtr_t wifiManager, LoggerPtr_t logger, std::function<bool()> shouldSkipPolling, const char* connectionString) :  _logger(logger), _shouldSkipPolling(shouldSkipPolling), _azureIoTHubDeviceConnectionString(connectionString)
{
	wifiManager->RegisterClient([this](ConnectionStatus status) { UpdateStatus(status); });
}

void AzureIoTHubManager::HandleCommand(const String & commandName) const
{
	_logger->WriteMessage("Received command: ");
	_logger->WriteMessage(commandName);
	int commandId = 0;

	if (commandName == "Open")
		commandId = 1;
	else if (commandName == "Close")
		commandId = 1;
	else if (commandName == "Stop")
		commandId = 2;

	if (commandId == 0)
	{
		_logger->WriteMessage("Invalid command, commands are case sensitive. [Open, Close, Stop]");
		return;
	}

	_pubsub.NotifyAll(commandName, commandId);
}


bool AzureIoTHubManager::CheckTimeInitiated()
{
	if ( _isTimeInitiated)
		return true;
	
	if (_IsInitTime == false)
		return false;
	
	time_t epochTime;

	epochTime = time(nullptr);

	if (epochTime == 0)
	{
		_logger->WriteErrorMessage("Fetching NTP epoch time failed!", 5);
		delay(100);
		return false;
	}
	//else
	
	_logger->WriteMessage("Fetched NTP epoch time is: ");
	_logger->WriteMessage(epochTime);
	_isTimeInitiated = true;
	return true;
}

bool AzureIoTHubManager::CheckIoTHubClientInitiated()
{
	if (_isIotHubClientInitiated)
		return true;

	_iotHubClient.begin(_sslWiFiClient);

	if (!AzureIoTHubInit(_azureIoTHubDeviceConnectionString))
		return false;
	_isIotHubClientInitiated = true;
	return true;
}



void AzureIoTHubManager::Loop()
{
	if (CheckTimeInitiated() == false) //can't do anything before setting the time
		return;
	if (CheckIoTHubClientInitiated() == false)
		return;

	if (_shouldSkipPolling && _shouldSkipPolling()) //skip polling AzureIoTHub
		return;

	if ((millis() - _loopStartTime) < 3000)
		return;
	_loopStartTime = millis();
	AzureIoTHubLoop();
}

void AzureIoTHubManager::UpdateGateStatus( char *deviceId, String status) const
{
	if (_isIotHubClientInitiated)
		AzureIoTHubSendMessage(deviceId, status.c_str());
}

void AzureIoTHubManager::UpdateStatus(ConnectionStatus status)
{
	if (status.IsJustConnected() && !_IsInitTime) //new connection, only once
	{
		configTime(0, 0, "pool.ntp.org", "time.nist.gov");
		_IsInitTime = true;
	}
}

