#include "ap_nif.hh"
#include "enodeb_nif.cc"


double // for WiFi RAT
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
WifiInstantaneousThputEstimate(measurementTimeWindowStats_t stats)
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








uint32_t // for LTE RAT
GetTbsFromMcs (uint16_t mcs)
{
  return(TransportBlockSizeTable[ENODEB_NUM_AVAILABLE_PRBS-1][McsToItbs[mcs]]);
}


//@Controller and @EnodeB
double
LteInstantaneousThputEstimate(measurementTimeWindowStats_t stats) 
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
