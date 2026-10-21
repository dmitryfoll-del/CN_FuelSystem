class cn_FuelConfigStructure
{
	int SPAWN_CHANCE_PERCENT;
	int MIN_FUEL_LITRES;
	int MAX_FUEL_LITRES;
	float PUMP_FUEL_COEFFICIENT;

	void cn_FuelConfigStructure()
	{
		SPAWN_CHANCE_PERCENT = 65;
		MIN_FUEL_LITRES = 20;
		MAX_FUEL_LITRES = 250;
		PUMP_FUEL_COEFFICIENT = 50.0;
	}
}

class cn_FuelStationData
{
	string POSITION;
	int CURRENT_FUEL;

	void cn_FuelStationData(string pos, int fuel)
	{
		POSITION = pos;
		CURRENT_FUEL = fuel;
	}
}

class cn_FuelServerDatabase
{
	ref array<ref cn_FuelStationData> STATIONS_DATA;

	void cn_FuelServerDatabase()
	{
		STATIONS_DATA = new array<ref cn_FuelStationData>;
	}
}

modded class Land_FuelStation_Feed
{
	private static ref cn_FuelConfigStructure cn_ServerSettings;
	private static ref cn_FuelServerDatabase cn_ServerDB;
	private static bool cn_IsSystemInitialized = false;
	private static bool cn_IsMapScannedForPumps = false;
	
	private static string cn_ConfigDir = "$profile:ColdNight_SRV_Data\\FuelSystem";
	private static string cn_ConfigFile = cn_ConfigDir + "\\CN_FuelFeedConfig.json"; 
	private static string cn_DataFile = cn_ConfigDir + "\\CN_FuelData.json";       

	override void EEInit()
	{
		super.EEInit();

		if (GetGame().IsServer())
		{
			cn_InitializeFuelSystem();
			cn_ControlFuelOnSpawn();
		}
	}

	static void cn_InitializeFuelSystem()
	{
		if (cn_IsSystemInitialized) return;

		if (!FileExist(cn_ConfigDir))
		{
			MakeDirectory(cn_ConfigDir);
		}

		cn_ServerSettings = new cn_FuelConfigStructure();
		if (FileExist(cn_ConfigFile))
		{
			JsonFileLoader<cn_FuelConfigStructure>.JsonLoadFile(cn_ConfigFile, cn_ServerSettings);
		}
		else
		{
			JsonFileLoader<cn_FuelConfigStructure>.JsonSaveFile(cn_ConfigFile, cn_ServerSettings);
		}

		cn_ServerDB = new cn_FuelServerDatabase();
		if (FileExist(cn_DataFile))
		{
			JsonFileLoader<cn_FuelServerDatabase>.JsonLoadFile(cn_DataFile, cn_ServerDB);
		}
		else
		{
			JsonFileLoader<cn_FuelServerDatabase>.JsonSaveFile(cn_DataFile, cn_ServerDB);
		}

		cn_IsSystemInitialized = true;
		cn_ScanMapAndAddBonusFuel();
	}

	static void cn_ScanMapAndAddBonusFuel()
	{
		if (cn_IsMapScannedForPumps || !cn_ServerDB || !cn_ServerSettings) return;

		if (!cn_ServerDB.STATIONS_DATA || cn_ServerDB.STATIONS_DATA.Count() <= 0)
		{
			cn_IsMapScannedForPumps = true;
			return;
		}

		string cn_pump_classname = "CN_OilPump"; 
		if (Class.CastTo(cn_pump_classname, "CN_MachineConfigManager") && CN_MachineConfigManager.GetOptions())
		{
			cn_pump_classname = CN_MachineConfigManager.GetOptions().GetOilPumpClassname(); 
		}

		array<Object> cn_found_pumps = new array<Object>;
		GetGame().GetObjectsOfName(cn_pump_classname, cn_found_pumps);

		int cn_pump_count = cn_found_pumps.Count();
		if (cn_pump_count > 0)
		{
			float cn_total_mined_litres = cn_pump_count * cn_ServerSettings.PUMP_FUEL_COEFFICIENT;
			int cn_stations_count = cn_ServerDB.STATIONS_DATA.Count();
			int cn_fuel_per_station = Math.Round(cn_total_mined_litres / cn_stations_count);

			Print(string.Format("[CN_FuelSystem] Найдено вышек: %1. Всего добыто: %2 л. Доля одной АЗС: %3 л.", cn_pump_count, cn_total_mined_litres, cn_fuel_per_station));

			for (int i = 0; i < cn_stations_count; i++)
			{
				int cn_roll = Math.RandomInt(0, 100);
				if (cn_roll < cn_ServerSettings.SPAWN_CHANCE_PERCENT)
				{
					cn_ServerDB.STATIONS_DATA.Get(i).CURRENT_FUEL += cn_fuel_per_station;
					if (cn_ServerDB.STATIONS_DATA.Get(i).CURRENT_FUEL > cn_ServerSettings.MAX_FUEL_LITRES)
					{
						cn_ServerDB.STATIONS_DATA.Get(i).CURRENT_FUEL = cn_ServerSettings.MAX_FUEL_LITRES;
					}
				}
			}
			
			// Бонус от вышек при старте пишем на диск сразу, так как это происходит один раз при загрузке
			cn_SaveServerDatabase();
		}

		cn_IsMapScannedForPumps = true;
	}

	void cn_ControlFuelOnSpawn()
	{
		if (!cn_ServerSettings || !cn_ServerDB) return;

		string cn_current_pos = GetPosition().ToString();
		cn_FuelStationData cn_saved_station = cn_FindStationData(cn_current_pos);

		if (cn_saved_station)
		{
			cn_SetServerFuel(cn_saved_station.CURRENT_FUEL);
		}
		else
		{
			int cn_initial_fuel = 0;
			int cn_roll = Math.RandomInt(0, 100);
			
			if (cn_roll < cn_ServerSettings.SPAWN_CHANCE_PERCENT)
			{
				cn_initial_fuel = Math.RandomIntIncluded(cn_ServerSettings.MIN_FUEL_LITRES, cn_ServerSettings.MAX_FUEL_LITRES);
			}

			cn_FuelStationData cn_new_station = new cn_FuelStationData(cn_current_pos, cn_initial_fuel);
			cn_ServerDB.STATIONS_DATA.Insert(cn_new_station);
			
			cn_SetServerFuel(cn_initial_fuel);
			cn_SaveServerDatabase();
		}
	}

	// Вызывается каждую секунду заправки. Теперь здесь ТОЛЬКО вывод логов в консоль
	override void DepleteFuel(float amount)
	{
		super.DepleteFuel(amount);

		if (GetGame().IsServer())
		{
			string cn_current_pos = GetPosition().ToString();
			Print(string.Format("[CN_FuelSystem] Идет отток топлива из колонки %1. Текущий остаток в памяти: %2 л.", cn_current_pos, cn_GetServerFuel()));
		}
	}

	// ТРИГГЕР ЗАВЕРШЕНИЯ ВЗАИМОДЕЙСТВИЯ: Игрок перестал заправляться, сеть синхронизировалась
	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		if (GetGame().IsServer())
		{
			cn_UpdateAndSaveStationMemory();
		}
	}

	// ТРИГГЕР ЗАЩИТЫ ОТ КРАША: Выгрузка объекта из памяти сервера
	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);

		if (GetGame().IsServer())
		{
			cn_UpdateAndSaveStationMemory();
		}
	}

	// Вспомогательный метод: обновляет оперативную память базы данных и делает ОДИН цикл записи на диск
	private void cn_UpdateAndSaveStationMemory()
	{
		string cn_current_pos = GetPosition().ToString();
		cn_FuelStationData cn_station_data = cn_FindStationData(cn_current_pos);

		if (cn_station_data)
		{
			// Проверяем, изменилось ли значение в памяти по сравнению с тем, что было сохранено
			if (cn_station_data.CURRENT_FUEL != cn_GetServerFuel())
			{
				cn_station_data.CURRENT_FUEL = cn_GetServerFuel();
				cn_SaveServerDatabase(); // Пишем на диск ОДИН РАЗ по окончании заправки
				
				Print(string.Format("[CN_FuelSystem] Заправка завершена. Результат сохранен на диск для колонки %1. Итоговый остаток: %2 л.", cn_current_pos, cn_station_data.CURRENT_FUEL));
			}
		}
	}

	static cn_FuelStationData cn_FindStationData(string cn_pos_target)
	{
		if (!cn_ServerDB || !cn_ServerDB.STATIONS_DATA) return null;

		for (int i = 0; i < cn_ServerDB.STATIONS_DATA.Count(); i++)
		{
			if (cn_ServerDB.STATIONS_DATA.Get(i).POSITION == cn_pos_target)
			{
				return cn_ServerDB.STATIONS_DATA.Get(i);
			}
		}
		return null;
	}

	static void cn_SaveServerDatabase()
	{
		if (cn_IsSystemInitialized && cn_ServerDB)
		{
			JsonFileLoader<cn_FuelServerDatabase>.JsonSaveFile(cn_DataFile, cn_ServerDB);
		}
	}
}
