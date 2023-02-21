#ifndef CONSTFEMSPATHS_H
#define CONSTFEMSPATHS_H
static const QString DATA_ACCESS_STRING_FEMS = "value";
static const QString SUM_STATE = "_sum/State";

static const QString ESS_SOC = "_sum/EssSoc";
static const QString ESS_ACTIVE_POWER = "_sum/EssActivePower";
static const QString ESS_ACTIVE_POWER_L1 = "_sum/EssActivePowerL1";
static const QString ESS_ACTIVE_POWER_L2 = "_sum/EssActivePowerL2";
static const QString ESS_ACTIVE_POWER_L3 = "_sum/EssActivePowerL3";

static const QString ESS_ACTIVE_CHARGE_ENERGY = "_sum/EssActiveChargeEnergy";
static const QString ESS_ACTIVE_DISCHARGE_ENERGY =
        "_sum/EssActiveDischargeEnergy";

static const QString ESS_CAPACITY = "ess0/Capacity";

static const QString GRID_ACTIVE_POWER = "_sum/GridActivePower";
static const QString GRID_ACTIVE_POWER_L1 = "_sum/GridActivePowerL1";
static const QString GRID_ACTIVE_POWER_L2 = "_sum/GridActivePowerL1";
static const QString GRID_ACTIVE_POWER_L3 = "_sum/GridActivePowerL1";

static const QString GRID_BUY_ACTIVE_ENERGY = "_sum/GridBuyActiveEnergy";
static const QString GRID_SELL_ACTIVE_ENERGY = "_sum/GridSellActiveEnergy";
static const QString GRID_PRODUCTION_ACTIVE_ENERGY =
        "_sum/ProductionActiveEnergy";
static const QString GRID_PRODUCTION_ACTIVE_AC_ENERGY =
        "_sum/ProductionAcActiveEnergy";
static const QString GRID_PRODUCTION_ACTIVE_DC_ENERGY =
        "_sum/ProductionDcActiveEnergy";
static const QString GRID_CONSUMPTION_ACTIVE_ENERGY =
        "_sum/ConsumptionActiveEnergy";

static const QString PRODCUTION_ACTIVE_POWER = "_sum/ProductionActivePower";
static const QString PRODUCTION_ACTIVE_AC_POWER =
        "_sum/ProductionAcActivePower";
static const QString PRODUCTION_ACTIVE_DC_POWER =
        "_sum/ProductionDcActualPower";
static const QString PRODUCTION_ACTIVE_AC_POWER_L1 =
        "_sum/ProductionAcActivePowerL1";
static const QString PRODUCTION_ACTIVE_AC_POWER_L2 =
        "_sum/ProductionAcActivePowerL2";
static const QString PRODUCTION_ACTIVE_AC_POWER_L3 =
        "_sum/ProductionAcActivePowerL3";

static const QString CONSUMPTION_ACTIVE_POWER = "_sum/ConsumptionActivePower";
static const QString CONSUMPTION_ACTIVE_AC_POWER_L1 =
        "_sum/ConsumptionActivePowerL1";
static const QString CONSUMPTION_ACTIVE_AC_POWER_L2 =
        "_sum/ConsumptionActivePowerL2";
static const QString CONSUMPTION_ACTIVE_AC_POWER_L3 =
        "_sum/ConsumptionActivePowerL3";

// EITHER  USE METER 0, 1 or 2 depending on MeterType

static const QString CURRENT_PHASE_1 = "CurrentL1";
static const QString CURRENT_PHASE_2 = "CurrentL2";
static const QString CURRENT_PHASE_3 = "CurrentL3";
static const QString FREQUENCY = "Frequency";

static const QString METER_0 = "meter0";
static const QString METER_1 = "meter1";
static const QString METER_2 = "meter2";
static const QString SKIP = "Skipping_No_Meter_Found";
static const QString FEMS_PLUGIN_STORAGE_ID = "FEMS_PLUGIN_STORAGE";


#endif // CONSTFEMSPATHS_H
