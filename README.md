# aquamet
Technology agnostic link quality metrics for radio access networks

# Wifi radio access technology

@ Access point

ApNif

A single object of ApNif runs at the AP.
These are the function that need to hooked to events happenning at the AP
AddAssociatedSta: To be called wen a new station is associated with this AP.
RecvStaMeasureReport: To be called when a measurement report containing neighbour signal strength information is received at the AP (supported by 802.11k).
UpdateStaTrafficStats: To be called when a new packet is received from the packet gateway. 
GetLocalNetworkState: To be called from the controller to ask AP to send the current local network state.
RemoveDisassociatedSta: To be called when a previously associated station has been disassociated with this AP.  
PktSent: To be called when a downlink data packet is sent to a station by the AP
PktSuccessfullySent: To be called when an Ack is received uplink for a data packet sent. 
UpdateStatsAtEndOfMeasWindow: To be called when the measurement window timer fires on the AP

StaNode

An object of StaNodes is instanciated in ApNif at the AP for each associated station.
It maintains information about the input traffic statistics of each station node.
UpdateStatsAtEndOfMeasWindow: To be called for each station when the measurement window timer fires on the AP.

ApStaDownlink

An object of the AP-Sta downlink information instanciated at the ApNif at the AP for each associated or candidate link between the AP, its associated stations and neighbouring APs.
UpdateEstMcs: To be called for each link when the measurement report is received at the AP.
UpdateStatsAtEndOfMeasWindow: To be called when the measurement window timer fires on the AP.
GetMeasThputProb: To be called when the measurement window timer fires on the AP. It is called only for links between the AP on which this runs and its associated stations. 


@ Controller

WifiInstantaneousThputEstimate: This function estimates the instaneous attainable throughput for the given input of AP-station association sets. 


# LTE radio access technology

@ Access point

EnodebNif

A single object of EnodebNif runs at the EnodeB.
These are the function that need to hooked to events happenning at the EnodeB
AddAssociatedUe: To be called when a new UE is associated with this EnodeB.
RecvUeMeasureReport: To be called when a measurement report containing neighbour signal strength information is received at the EnodeB.
UpdateUeTrafficStats: To be called when a new packet is received from the packet gateway. 
GetLocalNetworkState: To be called from the controller to ask EnodeB to send the current local network state.
RemoveDisassociatedUe: To be called when a previously associated UE has been disassociated with this EnodeB.  
PktSent: To be called when a downlink data packet is sent to a UE by the EnodeB
PktSuccessfullySent: To be called when an Ack is received uplink for a data packet sent. 
UpdateStatsAtEndOfMeasWindow: To be called when the measurement window timer fires on the EnodeB

UeNode

An object of UeNodes is instanciated in EnodeBNif at the EnodeB for each associated UE.
It maintains information about the input traffic statistics of each UE node.
UpdateStatsAtEndOfMeasWindow: To be called for each UE when the measurement window timer fires on the EnodeB.

EnodebUeDownlink

An object of the EnodeB-UE downlink information instanciated at the EnodebNif at the EnodeB for each associated or candidate link between the EnodeB, its associated UEs and neighbouring EnodeBs.
UpdateEstMcs: To be called for each link when the measurement report is received at the EnodeB.
UpdateStatsAtEndOfMeasWindow: To be called when the measurement window timer fires on the EnodeB.
GetMeasThputProb: To be called when the measurement window timer fires on the EnodeB. It is called only for links between the EnodeB on which this runs and its associated stations. 


@ Controller

LteInstantaneousThputEstimate: This function estimates the instaneous attainable throughput for the given input of EnodeB-UE association sets. 


