/*
	Copyright 2017 - 2018 Danny Bokma	danny@diebie.nl
	Copyright 2019 - 2020 Kevin Dionne	kevin.dionne@ennoid.me

	This file is part of the DieBieMS/ENNOID-BMS firmware.

	The DieBieMS/ENNOID-BMS firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The DieBieMS/ENNOID-BMS firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "modStateOfCharge.h"

modStateOfChargeStructTypeDef modStateOfChargeGeneralStateOfCharge;
modPowerElectronicsPackStateTypedef *modStateOfChargePackStateHandle;
modConfigGeneralConfigStructTypedef *modStateOfChargeGeneralConfigHandle;
uint32_t modStateOfChargeLargeCoulombTick;
uint32_t modStateOfChargeStoreSoCTick;
float modStateOfChargeDoDAccum = 0.0f;
float modStateOfChargeDoDPeriod = 0.0f; 
bool modStateOfChargePowerDownSavedFlag = false;

// V: 2.900 SoC:0.00 
// V: 2.950 SoC:1.00 
// V: 3.000 SoC:2.00 
// V: 3.050 SoC:3.00 
// V: 3.100 SoC:4.00 
// V: 3.150 SoC:5.00 
// V: 3.200 SoC:6.00 
// V: 3.250 SoC:7.00 
// V: 3.300 SoC:8.00 
// V: 3.350 SoC:10.00 
// V: 3.400 SoC:15.00 
// V: 3.450 SoC:20.00 
// V: 3.500 SoC:28.00 
// V: 3.550 SoC:38.00 
// V: 3.600 SoC:45.00 
// V: 3.650 SoC:50.00 
// V: 3.700 SoC:57.00 
// V: 3.750 SoC:62.00 
// V: 3.800 SoC:67.00 
// V: 3.850 SoC:75.00 
// V: 3.900 SoC:78.00 
// V: 3.950 SoC:85.00 
// V: 4.000 SoC:90.00 
// V: 4.050 SoC:95.00 
// V: 4.100 SoC:99.50 
// V: 4.150 SoC:100.00 

float voltageTable[] = {
    2.900, 2.925, 2.950, 2.975, 3.000, 3.025, 3.050, 3.075, 3.100, 3.125,
    3.150, 3.175, 3.200, 3.225, 3.250, 3.275, 3.300, 3.325, 3.350, 3.375,
    3.400, 3.425, 3.450, 3.475, 3.500, 3.525, 3.550, 3.575, 3.600, 3.625,
    3.650, 3.675, 3.700, 3.725, 3.750, 3.775, 3.800, 3.825, 3.850, 3.875,
    3.900, 3.925, 3.950, 3.975, 4.000, 4.025, 4.050, 4.075, 4.100, 4.125,
    4.150, 4.175
};

float SoCTable[] = {
    0.00, 0.50, 1.00, 1.50, 2.00, 2.50, 3.00, 3.50, 4.00, 4.50, 5.00, 5.50,
    6.00, 6.50, 7.00, 7.50, 8.00, 9.00, 10.00, 12.50, 15.00, 17.50, 20.00,
    24.00, 28.00, 33.00, 38.00, 41.50, 45.00, 47.50, 50.00, 53.50, 57.00,
    59.50, 62.00, 64.50, 67.00, 71.00, 75.00, 76.50, 78.00, 81.50, 85.00,
    87.50, 90.00, 92.50, 95.00, 97.25, 99.50, 99.75, 100.00, 100.00
};


modStateOfChargeStructTypeDef* modStateOfChargeInit(modPowerElectronicsPackStateTypedef *packState, modConfigGeneralConfigStructTypedef *generalConfigPointer){
	modStateOfChargePackStateHandle = packState;
	modStateOfChargeGeneralConfigHandle = generalConfigPointer;
	driverSWStorageManagerStateOfChargeStructSize = (sizeof(modStateOfChargeStructTypeDef)/sizeof(uint16_t)); // Calculate the space needed for the config struct in EEPROM
	
	modStateOfChargeLargeCoulombTick = HAL_GetTick();
	modStateOfChargeStoreSoCTick = HAL_GetTick();
	
	return &modStateOfChargeGeneralStateOfCharge;
};

void modStateOfChargeProcess(void){
	modStateOfChargeEnhanceCoulombCounting();
};

//Implement algorithm from article: https://www.analog.com/en/technical-articles/a-closer-look-at-state-of-charge-and-state-health-estimation-tech.html
void modStateOfChargeEnhanceCoulombCounting(void){
	// Calculate accumulated energy
	uint32_t dt = (HAL_GetTick() - modStateOfChargeLargeCoulombTick); 
	modStateOfChargeLargeCoulombTick = HAL_GetTick();

	modStateOfChargeStructTypeDef lastGeneralStateOfCharge;
	lastGeneralStateOfCharge = modStateOfChargeGeneralStateOfCharge;

	//Calculate Depth of discharge for a period of time
	modStateOfChargeDoDPeriod = -(dt*modStateOfChargePackStateHandle->packCurrent)/(3600*1000)/modStateOfChargeGeneralConfigHandle->batteryCapacity * 100.0f ;

	//Update operating efficiency based on current.  
	float n = (modStateOfChargePackStateHandle->packCurrent > 0) ? NC : ND;
	
	//Update DOD Accumulate
	modStateOfChargeDoDAccum += n*modStateOfChargeDoDPeriod;
	
	// Calculate state of charge
	modStateOfChargeGeneralStateOfCharge.generalStateOfCharge = modStateOfChargeGeneralStateOfCharge.generalStateOfHealth - modStateOfChargeDoDAccum;
	modStateOfChargeGeneralStateOfCharge.remainingCapacityAh = modStateOfChargeGeneralConfigHandle->batteryCapacity * modStateOfChargeGeneralStateOfCharge.generalStateOfCharge / 100.0f; 
	
	// Cap the max stored energy to the configured battery capacity and SoC value.
	if(modStateOfChargeGeneralStateOfCharge.remainingCapacityAh > modStateOfChargeGeneralConfigHandle->batteryCapacity)
		modStateOfChargeGeneralStateOfCharge.remainingCapacityAh = modStateOfChargeGeneralConfigHandle->batteryCapacity;
	if(modStateOfChargeGeneralStateOfCharge.remainingCapacityAh < 0.0f)
		modStateOfChargeGeneralStateOfCharge.remainingCapacityAh = 0.0f;
	if(modStateOfChargeGeneralStateOfCharge.generalStateOfCharge >= 100.0f)
		modStateOfChargeGeneralStateOfCharge.generalStateOfCharge = 100.0f;
	if(modStateOfChargeGeneralStateOfCharge.generalStateOfCharge < 0.0f)
		modStateOfChargeGeneralStateOfCharge.generalStateOfCharge = 0.0f;

	//Update packState
	modStateOfChargePackStateHandle->SoC = modStateOfChargeGeneralStateOfCharge.generalStateOfCharge;
	modStateOfChargePackStateHandle->SoCCapacityAh = modStateOfChargeGeneralStateOfCharge.remainingCapacityAh;
	
	// Store SoC every 'stateOfChargeStoreInterval'
	if(modDelayTick1ms(&modStateOfChargeStoreSoCTick,modStateOfChargeGeneralConfigHandle->stateOfChargeStoreInterval) && !modStateOfChargePowerDownSavedFlag && (lastGeneralStateOfCharge.remainingCapacityAh != modStateOfChargeGeneralStateOfCharge.remainingCapacityAh)){
		modStateOfChargeStoreStateOfCharge();	
	}
	// Compare calculated SOC to simple linear calculation and make adjustments 
	modStateOfChargeVoltageToSoC();
}

bool modStateOfChargeStoreAndLoadDefaultStateOfCharge(void){
	bool returnVal = false;
	modStateOfChargeStructTypeDef defaultStateOfCharge;
	if(driverSWStorageManagerStateOfChargeEmpty){
 		// TODO_EEPROM		
		defaultStateOfCharge.generalStateOfCharge = 100.0f;
		defaultStateOfCharge.generalStateOfHealth = 100.0f;
		defaultStateOfCharge.remainingCapacityAh = modStateOfChargeGeneralConfigHandle->batteryCapacity;
		defaultStateOfCharge.remainingCapacityWh = 0.0f;

		driverSWStorageManagerStateOfChargeEmpty = false;
   		driverSWStorageManagerStoreStruct(&defaultStateOfCharge,STORAGE_STATEOFCHARGE);
	}
	
	modStateOfChargeLoadStateOfCharge();
	return returnVal;
};

bool modStateOfChargeStoreStateOfCharge(void){
	return driverSWStorageManagerStoreStruct(&modStateOfChargeGeneralStateOfCharge,STORAGE_STATEOFCHARGE);
};

bool modStateOfChargeLoadStateOfCharge(void){
	return driverSWStorageManagerGetStruct(&modStateOfChargeGeneralStateOfCharge,STORAGE_STATEOFCHARGE);
};

bool modStateOfChargePowerDownSave(void) {
	if(!modStateOfChargePowerDownSavedFlag) {
		modStateOfChargePowerDownSavedFlag = true;
		modStateOfChargeStoreStateOfCharge();
		// TODO_EEPROM
		return true;
	}else
		return false;
};

void modStateOfChargeVoltageEvent(modStateOfChargeVoltageEventTypeDef eventType) {
	switch(eventType) {
		case EVENT_EMPTY:
			break;
		case EVENT_FULL:
			modStateOfChargeGeneralStateOfCharge.remainingCapacityAh = modStateOfChargeGeneralConfigHandle->batteryCapacity;
			break;
		default:
			break;
	}
};

void modStateOfChargeVoltageToSoC(void){
	for (int i = 0; i < sizeof(voltageTable)/sizeof(voltageTable[i]); i++){
		if (fabsf(modStateOfChargePackStateHandle->cellVoltageAverage - voltageTable[i]) < 0.0125 ){
			if(fabsf(modStateOfChargePackStateHandle->SoC - SoCTable[i]) > 2.5){ 
				modCommandsPrintf("modStateOfChargeVoltageToSoC: Update SoC based on Voltage table");
				modStateOfChargeDoDAccum = modStateOfChargeGeneralStateOfCharge.generalStateOfHealth - SoCTable[i];
			}
			return;
		}	
	}
	return;
}




