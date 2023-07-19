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
float voltageTable[] = 	{2.9, 2.95, 3.0, 3.05, 3.1, 3.15, 3.2, 3.25, 3.3, 3.35, 3.4 , 3.45, 3.5 , 3.55, 3.6 , 3.65, 3.7 , 3.75, 3.8 , 3.85, 3.9 , 3.95, 4.0 , 4.05, 4.1 , 4.15 , 4.2  }; // voltage values
float SoCTable[] = 		{0.0, 1.0 , 2.0, 3.0 , 4.0, 5.0 , 6.0, 7.0 , 8.0, 10.0, 15.0, 20.0,	28.0, 38.0, 45.0, 50.0, 57.0, 62.0, 67.0, 75.0, 78.0, 85.0, 90.0, 95.0, 99.5, 100.0, 100.0}; // Corresponding SoC values


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
		modCommandsPrintf("modStateOfCharge: Update and store SoC");	
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
	
	// modStateOfChargeStructTypeDef tempStateOfCharge;
	// driverSWStorageManagerGetStruct(&tempStateOfCharge,STORAGE_STATEOFCHARGE);
	
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
		if (fabsf(modStateOfChargePackStateHandle->cellVoltageAverage - voltageTable[i]) < 0.025 ){
			if(fabsf(modStateOfChargePackStateHandle->SoC - SoCTable[i]) > 3){ 
				modStateOfChargeDoDAccum = modStateOfChargeGeneralStateOfCharge.generalStateOfHealth - SoCTable[i];
			}
			return;
		}	
	}
	return;
}




