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

#include "ap_nif.hh"

//---------------------------------------------------
// class ApNif // there is one running at each eNodeB

// Constructor
ApNif::ApNif (deviceid_t apid)
{
	//current_num_associated_stas = 0;
	// This does shallow copy. Check if this is OK. 
	// It will be if there are no pointers in this structure
	myId = apid;
//	measurement_time_window = 200*1000;
//	num_meas_wind_in_slide_wind = 200;
	num_pkts_sent = 0;
    num_pkts_successfully_sent = 0;
}

// Destructor
ApNif::~ApNif ()
{

}

uint8_t
ApNif::compare(deviceid_t d1, deviceid_t d2)
{
	return(d1.id == d2.id ? 1 : 0);
}

void 
ApNif::AddAssociatedSta (deviceid_t sta_id) // call when a new sta is associated to this ap
{
	StaNodeInfo u = StaNodeInfo(sta_id, myId); 
	stanodes.push_back(u);  
	// Add an associated link to this sta
	ApStaDownlink dl = ApStaDownlink(sta_id, myId);
	dl.associatedLink = 1;
	dlink.push_back(dl);
}

void 
ApNif::RecvStaMeasureReport(staMeasureReport_t report) // call when a measurement report is received from an associated sta. 
{
	for(uint16_t i=0; i<report.num_of_nbrs; i++)
	{
		// Search if link in measurement report has een created in the NIF
		for(std::vector<ApStaDownlink>::iterator it = dlink.begin(); it != dlink.end(); ++it) 
		{				
			if(compare(*report.nbr_ids, it->ap_id) && compare(report.staid, it->sta_id))
			{
				// If yes then update it
				it->sum_est_mcs = it->sum_est_mcs + *report.avg_nbr_mcs;		
			}
		}
		// If I could not find it then create this new candidate link 
		ApStaDownlink dl = ApStaDownlink(report.staid, *report.nbr_ids);
		dlink.push_back(dl);
		report.nbr_ids++;
		report.avg_nbr_mcs++;
	}					
}
 
void 
ApNif::UpdateStaTrafficStats (staPktStats_t pkt_stats) // call when a new pkt is received from pgw  
{
	// find the sta this pkt is destined to. 
	for(std::vector<StaNodeInfo>::iterator it = stanodes.begin(); it != stanodes.end(); ++it) 
	{
		if(compare(pkt_stats.dest_sta_id, it->sta_id))
		{
			it->sum_pgw_recv_bytes += pkt_stats.num_bytes;
    		it->sum_pgw_recv_num_pkts += 1;
    		it->sum_pgw_recv_pkt_len += pkt_stats.pkt_len;
		}
	}
	// error: unassociated sta's pkt has been received
}

void // the corresponding data type should replace void here
ApNif::GetLocalNetworkState() // called from controller. Send all the node and link info to controller 
{
// dlink vector is sent to the controller through a socket connection  
}
 
 /*   
void 
ApNif::SetMeasureTimeWindow(uint64_t meas_time_wind) // called from controller
{
	measurement_time_window = meas_time_wind;
}

uint64_t 
ApNif::GetMeasureTimeWindow(void) // called from controller
{
    return(measurement_time_window);
}
    
void 
ApNif::SetNumOfMeasureTimeWindowsInSlideWindow (uint16_t n_mtw) // called from controller
{
    num_meas_wind_in_slide_wind = n_mtw;
}

uint16_t 
ApNif::GetNumOfMeasureTimeWindowsInSlideWindow (void) // called from controller
{
    return(num_meas_wind_in_slide_wind);
}
*/
    
void 
ApNif::RemoveDisassociatedSta (deviceid_t sta_id) // call on disassociation of sta to this ap
{
	// find and remove this sta. 
	for(std::vector<StaNodeInfo>::iterator it = stanodes.begin(); it != stanodes.end(); ++it) 
	{
		if(compare(sta_id, it->sta_id))
		{
			stanodes.erase(it);
		}
	}
	// find and remove all its links
	for(std::vector<ApStaDownlink>::iterator it = dlink.begin(); it != dlink.end(); ++it) 
	{
		if(compare(sta_id, it->sta_id))
		{
			dlink.erase(it);
		}
	}
}

// called every time a pkt is sent from AP
void
ApNif::PktSent ()
{
	num_pkts_sent++;
}

void
ApNif::PktSuccessfullySent (deviceid_t sta, uint32_t num_bytes)
{
	num_pkts_successfully_sent++;
	// find this UE's associated link
  	for(std::vector<ApStaDownlink>::iterator it = dlink.begin(); it != dlink.end(); ++it) 
  	{
    	if(compare(sta, it->sta_id) && compare(myId, it->ap_id) && (it->associatedLink))
    	{
      		it->sum_successfully_tx_bytes += num_bytes;
    	  break;
    	}
  	}
}

void
ApNif::UpdateStatsAtEndOfMeasWindow ()
{
	double delRate = (double)num_pkts_successfully_sent/(double)num_pkts_sent;
	pdr.push_back(delRate);
	pdr.erase(pdr.begin());
	num_pkts_sent = 0;
	num_pkts_successfully_sent = 0;
}


//---------------------------------------------------------------------------------------------------------------
StaNodeInfo::StaNodeInfo (deviceid_t staid, deviceid_t apid)
{
	sta_id = staid;
	associated_ap = apid;
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
StaNodeInfo::~StaNodeInfo ()
{

}

void 
StaNodeInfo::UpdateStatsAtEndOfMeasWindow (void)
{
	double arrivalRate_pktsPerS = (sum_pgw_recv_num_pkts*1000000/MEASUREMENT_TIME_WINDOW);
	double arrivalRate_kbps = (sum_pgw_recv_bytes/MEASUREMENT_TIME_WINDOW)*8.0*1000.0;
	double avg_pktLen = sum_pgw_recv_pkt_len/sum_pgw_recv_num_pkts;

	// Add latest measurement window valsta to the end of the sliding window vector 
    arr_rate_pkts_per_sec.push_back(arrivalRate_pktsPerS);
    arr_rate_kbps.push_back(arrivalRate_kbps);	
    arr_avg_pkt_len_bytes.push_back(avg_pktLen);

    // remove the first measurement window valsta from the beginning of the sliding window vector 
    arr_rate_pkts_per_sec.erase(arr_rate_pkts_per_sec.begin());
    arr_rate_kbps.erase(arr_rate_kbps.begin());
    arr_avg_pkt_len_bytes.erase(arr_avg_pkt_len_bytes.begin());

    // reset the online computation valstas
    sum_pgw_recv_bytes = 0;
    sum_pgw_recv_num_pkts = 0;
    sum_pgw_recv_pkt_len = 0;    
}

void 
StaNodeInfo::ClearInfo (void)
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
StaNodeInfo::UpdateAssociatedAp (deviceid_t updated_ap)
{
	associated_ap = updated_ap;
}

//----------------------------------------------------------------------------------------
//-----------------------------------
// class ApStaDownlink
// Constructor
ApStaDownlink::ApStaDownlink (deviceid_t staid, deviceid_t apid)
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

    sta_id = staid;
    ap_id = apid;

    associatedLink = 0;
}

// Destructor
ApStaDownlink::~ApStaDownlink ()
{

}

void 
ApStaDownlink::UpdateEstMcs (uint32_t est_mcs)
{
	sum_est_mcs = sum_est_mcs + est_mcs;
}

void 
ApStaDownlink::UpdateStatsAtEndOfMeasWindow ()
{
	if(associatedLink)
	{
		uint32_t tmp_meas_mcs = round((double)sum_meas_mcs/(double)num_pkts_for_mcs_measure);
		double tmp_meas_thput_kbps = (double)(sum_successfully_tx_bytes*8*1000)/(double)MEASUREMENT_TIME_WINDOW;		
		// Add latest measurement window valsta to the end of the sliding window vector 
	    measured_mcs.push_back(tmp_meas_mcs);
    	measured_thput.push_back(tmp_meas_thput_kbps);
    	// remove the first measurement window valsta from the beginning of the sliding window vector
	    measured_mcs.erase(measured_mcs.begin());
    	measured_thput.erase(measured_thput.begin());
    	// reset the online computation valstas
        sum_meas_mcs = 0;
		sum_successfully_tx_bytes = 0;
	}
	// Add latest measurement window valsta to the end of the sliding window vector 
	uint32_t tmp_est_mcs = round((double)sum_est_mcs/(double)num_measurement_reports_recv);
    estimated_mcs.push_back(tmp_est_mcs);
    // remove the first measurement window valsta from the beginning of the sliding window vector 
    estimated_mcs.erase(estimated_mcs.begin());	
    // reset the online computation valstas
    sum_est_mcs = 0;

}

void 
ApStaDownlink::ClearInfo (void)
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
ApStaDownlink::GetMeasThputProb (double thput_threshold)
{
	double prob;

	return(prob);
}   
























//-------------------------------------------------------------------------------

double
AckTime (uint32_t mcs)
{
	uint32_t rate_ack; 
	double t_ack;
	if(mcs >= 24000){
		rate_ack = 24000;
	}
	else{
		rate_ack = 2000;
	}
    t_ack = ((double)(L_ACK * 8 * 1000) /(double)rate_ack) + T_HEADER;
}


double
InstantaneousThputEstimate(measurementTimeWindowStats_t stats)
{
	// the first structure object in stats belongs to the ue whose attainable throughput is to be estimated
	double* arr_rate = stats.arr_rate;
	double* avg_pkt_len = stats.avg_pkt_len;
	uint32_t* mcs = stats.mcs;
	double den = 0;
	double estimated_thput = 0;
	for (uint16_t i=0; i<stats.num_active_stas; i++) 
	{
		double t_ack = AckTime ((*mcs)); // milli s

		den += (*arr_rate) * ((*avg_pkt_len)/(double)(*mcs) + DIFS + SIFS + T_HEADER + t_ack);
	  	arr_rate++;
		avg_pkt_len++;
        mcs++;
    }
	// Get stats from the first structure obkect which is the ue whose attainable throughput is to be measured
	arr_rate = stats.arr_rate;
	avg_pkt_len = stats.avg_pkt_len;
	mcs = stats.mcs;

	double thput_unsaturated = (*arr_rate) * (*avg_pkt_len) * stats.apPdr * 8 / 1000;
	double thput_saturated = (*arr_rate) * (*avg_pkt_len) * stats.apPdr * 8.0 * 1000.0 / den;
	printf("sat thput is %f\n", thput_saturated);
	printf("unsat thput is %f\n", thput_unsaturated);
	estimated_thput = MIN(thput_saturated, thput_unsaturated);
	printf("lambda is %f\n", estimated_thput);
	return(estimated_thput);
}

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