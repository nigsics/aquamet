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

#include "enodeb_nif.hh"

//---------------------------------------------------
// class EnodebNif // there is one running at each eNodeB

// Constructor
EnodebNif::EnodebNif (deviceid_t enodebid)
{
  //current_num_associated_ues = 0;
  // This does shallow copy. Check if this is OK. 
  // It will be if there are no pointers in this structure
  myId = enodebid;
//  measurement_time_window = 200*1000;
//  num_meas_wind_in_slide_wind = 200;
    num_pkts_sent = 0;
    num_pkts_successfully_sent = 0;
}

// Destructor
EnodebNif::~EnodebNif ()
{

}

uint8_t
EnodebNif::compare(deviceid_t d1, deviceid_t d2)
{
  return(d1.id == d2.id ? 1 : 0);
}

void 
EnodebNif::AddAssociatedUe (deviceid_t ue_id) // call when a new ue is associated to this enodeb
{
  UeNodeInfo u = UeNodeInfo(ue_id, myId); 
  uenodes.push_back(u);  
  // Add an associated link to this ue
  EnodebUeDownlink dl = EnodebUeDownlink(ue_id, myId);
  dl.associatedLink = 1;
  dlink.push_back(dl);
}

void 
EnodebNif::RecvUeMeasureReport(ueMeasureReport_t report) // call when a measurement report is received from an associated ue. 
{
  for(uint16_t i=0; i<report.num_of_nbrs; i++)
  {
    // Search if link in measurement report has een created in the NIF
    for(std::vector<EnodebUeDownlink>::iterator it = dlink.begin(); it != dlink.end(); ++it) 
    {       
      if(compare(*report.nbr_ids, it->enodeb_id) && compare(report.ueid, it->ue_id))
      {
        // If yes then update it
        it->sum_est_mcs = it->sum_est_mcs + *report.avg_nbr_mcs;    
      }
    }
    // If I could not find it then create this new candidate link 
    EnodebUeDownlink dl = EnodebUeDownlink(report.ueid, *report.nbr_ids);
    dlink.push_back(dl);
    report.nbr_ids++;
    report.avg_nbr_mcs++;
  }         
}
 
void 
EnodebNif::UpdateUeTrafficStats (uePktStats_t pkt_stats) // call when a new pkt is received from pgw  
{
  // find the ue this pkt is destined to. 
  for(std::vector<UeNodeInfo>::iterator it = uenodes.begin(); it != uenodes.end(); ++it) 
  {
    if(compare(pkt_stats.dest_ue_id, it->ue_id))
    {
      it->sum_pgw_recv_bytes += pkt_stats.num_bytes;
        it->sum_pgw_recv_num_pkts += 1;
        it->sum_pgw_recv_pkt_len += pkt_stats.pkt_len;
    }
  }
  // error: unassociated ue's pkt has been received
}

void // the corresponding data type should replace void here
EnodebNif::GetLocalNetworkState() // called from controller. Send all the node and link info to controller 
{
// dlink vector is sent to the controller through a socket connection  
}
 
 /*   
void 
EnodebNif::SetMeasureTimeWindow(uint64_t meas_time_wind) // called from controller
{
  measurement_time_window = meas_time_wind;
}

uint64_t 
EnodebNif::GetMeasureTimeWindow(void) // called from controller
{
    return(measurement_time_window);
}
    
void 
EnodebNif::SetNumOfMeasureTimeWindowsInSlideWindow (uint16_t n_mtw) // called from controller
{
    num_meas_wind_in_slide_wind = n_mtw;
}

uint16_t 
EnodebNif::GetNumOfMeasureTimeWindowsInSlideWindow (void) // called from controller
{
    return(num_meas_wind_in_slide_wind);
}
*/
    
void 
EnodebNif::RemoveDisassociatedUe (deviceid_t ue_id) // call on disassociation of ue to this enodeb
{
  // find and remove this ue. 
  for(std::vector<UeNodeInfo>::iterator it = uenodes.begin(); it != uenodes.end(); ++it) 
  {
    if(compare(ue_id, it->ue_id))
    {
      uenodes.erase(it);
    }
  }
  // find and remove all its links
  for(std::vector<EnodebUeDownlink>::iterator it = dlink.begin(); it != dlink.end(); ++it) 
  {
    if(compare(ue_id, it->ue_id))
    {
      dlink.erase(it);
    }
  }
}

// called every time a pkt is sent from AP
void
EnodebNif::PktSent ()
{
  num_pkts_sent++;
}

void
EnodebNif::PktSuccessfullySent (deviceid_t ue, uint32_t num_bytes)
{
  num_pkts_successfully_sent++;
  // find this UE's associated link
  for(std::vector<EnodebUeDownlink>::iterator it = dlink.begin(); it != dlink.end(); ++it) 
  {
    if(compare(ue, it->ue_id) && compare(myId, it->enodeb_id) && (it->associatedLink))
    {
      it->sum_successfully_tx_bytes += num_bytes;
      break;
    }
  }
}

void
EnodebNif::UpdateStatsAtEndOfMeasWindow ()
{
  num_pkts_sent = 0;
  num_pkts_successfully_sent = 0;
}

//---------------------------------------------------------------------------------------------------------------
UeNodeInfo::UeNodeInfo (deviceid_t ueid, deviceid_t enodebid)
{
  ue_id = ueid;
  associated_enodeb = enodebid;
  // Initialize the sliding window with all zeros to begin with.
  for(uint16_t i=0; i<NUM_MEAS_WIND_IN_SLIDE_WIND; i++)
  {
      arr_rate_pkts_per_sec.push_back(0);
      arr_rate_kbps.push_back(0); 
      arr_avg_pkt_len_bytes.push_back(0);
  }

    sum_pgw_recv_bytes = 0;
    sum_pgw_recv_num_pkts = 0;
    sum_pgw_recv_pkt_len = 0;   
}

//Destructor
UeNodeInfo::~UeNodeInfo ()
{

}

void 
UeNodeInfo::UpdateStatsAtEndOfMeasWindow (void)
{
  double arrivalRate_pktsPerS = (sum_pgw_recv_num_pkts*1000000/MEASUREMENT_TIME_WINDOW);
  double arrivalRate_kbps = (sum_pgw_recv_bytes/MEASUREMENT_TIME_WINDOW)*8.0*1000.0;
  double avg_pktLen = sum_pgw_recv_pkt_len/sum_pgw_recv_num_pkts;

  // Add latest measurement window value to the end of the sliding window vector 
    arr_rate_pkts_per_sec.push_back(arrivalRate_pktsPerS);
    arr_rate_kbps.push_back(arrivalRate_kbps);  
    arr_avg_pkt_len_bytes.push_back(avg_pktLen);

    // remove the first measurement window value from the beginning of the sliding window vector 
    arr_rate_pkts_per_sec.erase(arr_rate_pkts_per_sec.begin());
    arr_rate_kbps.erase(arr_rate_kbps.begin());
    arr_avg_pkt_len_bytes.erase(arr_avg_pkt_len_bytes.begin());

    // reset the online computation values
    sum_pgw_recv_bytes = 0;
    sum_pgw_recv_num_pkts = 0;
    sum_pgw_recv_pkt_len = 0;    
}

void 
UeNodeInfo::ClearInfo (void)
{
  for(std::vector<double>::iterator it = arr_rate_pkts_per_sec.begin(); it != arr_rate_pkts_per_sec.end(); ++it) 
  {
    *it = 0;
  }
  for(std::vector<double>::iterator it = arr_rate_kbps.begin(); it != arr_rate_kbps.end(); ++it) 
  {
    *it = 0;
  }
  for(std::vector<double>::iterator it = arr_avg_pkt_len_bytes.begin(); it != arr_avg_pkt_len_bytes.end(); ++it) 
  {
    *it = 0;
  }

    sum_pgw_recv_bytes = 0;
    sum_pgw_recv_num_pkts = 0;
    sum_pgw_recv_pkt_len = 0;
}


void 
UeNodeInfo::UpdateAssociatedEnodeb (deviceid_t updated_enodeb)
{
  associated_enodeb = updated_enodeb;
}

//----------------------------------------------------------------------------------------
//-----------------------------------
// class EnodebUeDownlink
// Constructor
EnodebUeDownlink::EnodebUeDownlink (deviceid_t ueid, deviceid_t enodebid)
{
  // Initialize the sliding window with all zeros to begin with. 
  for(uint16_t i=0; i<NUM_MEAS_WIND_IN_SLIDE_WIND; i++)
  {
      estimated_mcs.push_back(0);
      measured_mcs.push_back(0);
      measured_thput.push_back(0);
  }
   
    sum_est_mcs = 0;
    sum_meas_mcs = 0; 
    sum_successfully_tx_bytes = 0;

    ue_id = ueid;
    enodeb_id = enodebid;

    associatedLink = 0;
}

// Destructor
EnodebUeDownlink::~EnodebUeDownlink ()
{

}

void 
EnodebUeDownlink::UpdateEstMcs (uint32_t est_mcs)
{
  sum_est_mcs = sum_est_mcs + est_mcs;
}

void 
EnodebUeDownlink::UpdateStatsAtEndOfMeasWindow ()
{
  if(associatedLink)
  {
    uint32_t tmp_meas_mcs = round((double)sum_meas_mcs/(double)num_pkts_for_mcs_measure);
    double tmp_meas_thput_kbps = (double)(sum_successfully_tx_bytes*8*1000)/(double)MEASUREMENT_TIME_WINDOW;    
    // Add latest measurement window value to the end of the sliding window vector 
      measured_mcs.push_back(tmp_meas_mcs);
      measured_thput.push_back(tmp_meas_thput_kbps);
      // remove the first measurement window value from the beginning of the sliding window vector
      measured_mcs.erase(measured_mcs.begin());
      measured_thput.erase(measured_thput.begin());
      // reset the online computation values
        sum_meas_mcs = 0;
    sum_successfully_tx_bytes = 0;
  }
  // Add latest measurement window value to the end of the sliding window vector 
  uint32_t tmp_est_mcs = round((double)sum_est_mcs/(double)num_measurement_reports_recv);
    estimated_mcs.push_back(tmp_est_mcs);
    // remove the first measurement window value from the beginning of the sliding window vector 
    estimated_mcs.erase(estimated_mcs.begin()); 
    // reset the online computation values
    sum_est_mcs = 0;

}

void 
EnodebUeDownlink::ClearInfo (void)
{
  for(std::vector<uint32_t>::iterator it = estimated_mcs.begin(); it != estimated_mcs.end(); ++it) 
  {
    *it = 0;
  }
  for(std::vector<uint32_t>::iterator it = measured_mcs.begin(); it != measured_mcs.end(); ++it) 
  {
    *it = 0;
  }
  for(std::vector<double>::iterator it = measured_thput.begin(); it != measured_thput.end(); ++it) 
  {
    *it = 0;
  } 

    sum_est_mcs = 0;
    sum_meas_mcs = 0;
    sum_successfully_tx_bytes = 0;
}  

double 
EnodebUeDownlink::GetMeasThputProb (double thput_threshold)
{
  double prob;

  return(prob);
} 



uint32_t 
GetTbsFromMcs (uint16_t mcs)
{
  return(TransportBlockSizeTable[ENODEB_NUM_AVAILABLE_PRBS-1][McsToItbs[mcs]]);
}


//@Controller and @EnodeB
double
InstantaneousThputEstimate(measurementTimeWindowStats_t stats) 
{
  // the first structure object in stats belongs to the ue whose attainable throughput is to be estimated
  double* arr_rate = stats.arr_rate;
  double* avg_pkt_len = stats.avg_pkt_len;
  uint32_t* mcs = stats.mcs;
  uint16_t num_HR = 0;
  double sum_LR_X = 0; 
  double est_resource_alloc = 0;
  double est_thput_rlc = 0;
  for (uint16_t i=0; i<stats.num_active_ues; i++ ) 
  {
    uint32_t Tbs = GetTbsFromMcs(*mcs); // will be 0 if set is empty
    // Number of TTIs needed to send one pkt
    double N = ceil((*avg_pkt_len)/(double)Tbs); // will be 0 if set is empty
    // Amount of resources requested by UE. units, TTIs/10ms or subframes/frame
    double X = (*arr_rate) * N; // will be 0 if either components are 0
    // Number of flows that are high rate flows. i.e. number of flows that request resources > their share.
    // Their share here is total resources / num. of active UEs. HEre shown as 10ms frame/ num. of active UEs
    if (X > FRAME_TIME_MILLI_S/stats.num_active_ues)
    {
      num_HR += 1;  
    }
    else
    {
      // Sum of the resources requested by UEs that are requesting less than their share. 
      sum_LR_X += X;
    }
    arr_rate++;
    avg_pkt_len++;
    mcs++;
  }

  // Get stats from the first structure obkect which is the ue whose attainable throughput is to be measured
  arr_rate = stats.arr_rate;
  avg_pkt_len = stats.avg_pkt_len;
  mcs = stats.mcs;

  uint32_t Tbs = GetTbsFromMcs(*mcs); // will be 0 if set is empty
  // Number of TTIs needed to send one pkt
  double N = ceil((*avg_pkt_len)/(double)Tbs);// will be 0 if set is empty
  // Amount of resources requested by UE. units, TTIs/10ms or subframes/frame
  double X = (*arr_rate) * N; // will be 0 if either components are 0
  // Resources actually allocated to this UE. unit, TTIs/10ms
  if (X <= (double)FRAME_TIME_MILLI_S/(double)stats.num_active_ues)
  {
    est_resource_alloc = X; 
  } 
  else
  {
    est_resource_alloc = MIN(X, (double)(NUM_TTI_IN_FRAME - sum_LR_X)/(double)(num_HR));
  }
  est_thput_rlc = MIN((est_resource_alloc * Tbs), ((*arr_rate) * (*avg_pkt_len) * 8 / NUM_TTI_IN_FRAME)); // Kbps 
  return(est_thput_rlc);
}


 

//@Controller and @EnodeB
// Gives P(lambda >= threshold)
double
ProbOfAchievingThreshold (double threshold, uint16_t len, double* thput_array)
{
  // arrange the throughput values in the array in ascending order 
  double *i, *j, *k;
  double temp;
  for(i = thput_array; i < thput_array + len; i++)
  {
    for(j = i + 1; j < thput_array + len; j++)
    {
      if(*i > *j)
      {
        temp = *i;
        *i = *j;
        *j = temp;
      }
    }
  }

  uint16_t count = 0;
  for (k = thput_array; k < thput_array + len; k++)
  {
    if (*k >= threshold)
    {
      break;
    } 
    count++;
  }
  return((double)(len - count)/(double)len);
}


int main ()
{
  return(1);
}


