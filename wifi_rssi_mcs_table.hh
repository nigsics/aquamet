#define RECV_NOISE -95.0
#define WIFI_STD "g20mhz"

// Std 802.11a/g 20 MHz
int16_t g_mcs_table[51] = {
	-1, -1, 0, 0, 1, 2, 2, 2, 2, 3, 3, 
	4, 4, 4, 4, 5, 5, 5, 6, 6, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7
}; 
/*
// std 802.11n 20 MHz and 40 MHz
int16_t n_mcs_table[2][51] = {
	// 20 MHz
	{-1, -1, 0, 0, 0, 1, 1, 1, 1, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 5, 5, 6,
		6, 6, 6, 6, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7},
		// 40 MHz
	{-1, -1, -1, -1, -1, 0, 0, 0, 1, 1, 1,
		1, 2, 2, 3, 3, 3, 3, 4, 4, 4,
		5, 5, 6, 6, 6, 6, 6, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7}
};*/

double g_mcs_coderate_table[8] = {1/2, 3/4, 1/2, 3/4, 1/2, 3/4, 2/3, 3/4};

uint32_t g_mcs_sendingrate_table[8] = {6000, 9000, 12000, 18000, 24000, 36000, 48000, 54000};

double
GetNoise()
{
	return (RECV_NOISE);
}

double
GetSnrFromRssi(double rssi_db)
{
	double snr_db = rssi_db - GetNoise();
	return (snr_db);
}

double 
GetMcsFromSnr(double snr_db)
{
	uint16_t snr_db_lower = (uint16_t)snr_db; 
	if (strcmp(WIFI_STD,"g20mhz") == 0)
	{
		return(g_mcs_table[snr_db_lower]);
	}
	/*
	else if (strcmp(WIFI_STD,"n20mhz") == 0)
	{
		return(n_mcs_table[0][snr_db_lower]);
	}
	else if (strcmp(WIFI_STD,"n40mhz") == 0)
	{
		return(n_mcs_table[0][snr_db_lower]);
	}
	*/
    // ERROR: Unrecognised 802.11 standard 
	return (-1);
}

double 
GetSendingRateFromMcs(uint16_t mcs)
{
	return(g_mcs_sendingrate_table[mcs]);
}

double
GetEstimatedSendingRateFromRssi(double rssi_db)
{
	return(GetSendingRateFromMcs(GetMcsFromSnr(GetSnrFromRssi(rssi_db))));
}

double
GetEstimatedMcsFromRssi(double rssi_db)
{
	return(GetMcsFromSnr(GetSnrFromRssi(rssi_db)));
}













