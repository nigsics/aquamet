/***Copyright 2017 RISE SICS AB

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ***/

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <iterator>

#define MAX_NUM_OF_ASSOCIATED_UE  100 
#define MAX_NUM_OF_ENODEB  50
#define MEASUREMENT_TIME_WINDOW 1000000 // microseconds
#define NUM_MEAS_WIND_IN_SLIDE_WIND 20  


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define DIFS 50
#define SIFS 10 
#define T_HEADER 56
#define L_ACK 14

typedef struct deviceid_t
{
  uint16_t id;
} deviceid_t;

typedef struct staPktStats_t{
    deviceid_t dest_sta_id;
    uint16_t pkt_len;
    uint16_t num_bytes; 
} staPktStats_t;

typedef struct staMeasureReport_t{
    deviceid_t staid;
    uint16_t num_of_nbrs;
    deviceid_t* nbr_ids;
    uint32_t* avg_nbr_mcs;
} staMeasureReport_t; 

typedef struct measurementTimeWindowStats_t{
    uint16_t num_active_stas;
    double* arr_rate;
    double* avg_pkt_len;
    uint32_t* mcs;
    double apPdr;
} measurementTimeWindowStats_t;



class ApStaDownlink
{
public:
    ApStaDownlink (deviceid_t,deviceid_t);
    ~ApStaDownlink ();

    deviceid_t sta_id;
    deviceid_t ap_id;
    uint8_t associatedLink; // 1 or 0
    std::vector<uint32_t> estimated_mcs;

    // for associated sta links only
    std::vector<double> measured_thput;
    std::vector<uint32_t> measured_mcs;
    // for online computation
    uint64_t sum_est_mcs;
    uint64_t sum_meas_mcs; 
    uint64_t sum_successfully_tx_bytes;
    uint16_t num_measurement_reports_recv;
    uint32_t num_pkts_for_mcs_measure;

    double GetMeasThputProb (double thput_threshold);
    void UpdateStatsAtEndOfMeasWindow (void);
    void UpdateEstMcs (uint32_t);
    void ClearInfo (void);  
};

class StaNodeInfo
{
public:
    StaNodeInfo (deviceid_t, deviceid_t);
    ~StaNodeInfo ();

    deviceid_t sta_id;
    deviceid_t associated_ap;  
    std::vector<double> arr_rate_pkts_per_sec;
    std::vector<double> arr_rate_kbps;
    std::vector<double> arr_avg_pkt_len_bytes;
    
    // only for updating, need not be sent, but have this here because it 
    // is a easier than having a separate graph or database to do this
    uint64_t sum_pgw_recv_bytes;
    uint64_t sum_pgw_recv_num_pkts;
    uint64_t sum_pgw_recv_pkt_len;
    
    void UpdateStatsAtEndOfMeasWindow ();
    void UpdateAssociatedAp (deviceid_t);
    void ClearInfo (void);
};


class ApNif // there is one running at each eNodeB
{
public:
    ApNif (deviceid_t);
    ~ApNif ();

public:    
    void AddAssociatedSta (deviceid_t); // call when a new sta is associated to this ap
    void RecvStaMeasureReport(staMeasureReport_t); // call when a measurement report is received from an associated sta. 
    void UpdateStaTrafficStats (staPktStats_t); // call when a new pkt is received from pgw  
    void GetLocalNetworkState(void); // called from controller. Send all the node and link info to controller 
    void SetMeasureTimeWindow(uint64_t); // called from controller
    uint64_t GetMeasureTimeWindow(void); // called from controller
    void SetNumOfMeasureTimeWindowsInSlideWindow (void); // called from controller
    uint16_t GetNumOfMeasureTimeWindowsInSlideWindow (void); // called from controller
    void RemoveDisassociatedSta (deviceid_t); // call on disassociation of sta to this ap
    void UpdateStatsAtEndOfMeasWindow (void); // call when measurement window timer fires. 
    void PktSuccessfullySent (deviceid_t, uint32_t);
    void PktSent (void);

    //static uint64_t measurement_time_window; // microseconds
    //static uint16_t num_meas_wind_in_slide_wind;  
    // create
    //uint16_t current_num_associated_stas;
    std::vector<StaNodeInfo> stanodes;
    std::vector<ApStaDownlink> dlink;
    std::vector<double> pdr;
    uint64_t num_pkts_sent;
    uint64_t num_pkts_successfully_sent;
    deviceid_t myId; // could be a uid or an ip addr. Don't know which one I need yet.

private:  
    uint8_t compare (deviceid_t,deviceid_t); 
};


