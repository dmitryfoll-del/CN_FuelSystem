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
		MAX_FUEL_LITRES = 250; // Используется и как начальный максимум, и как лимит вместимости АЗС
		PUMP_FUEL_COEFFICIENT = 50.0;
	}
}

// Класс данных для сохранения состояния одной конкретной АЗС
class cn_FuelStationData
{
	string POSITION; // Уникальный строковый ID колонки по ее координатам
	int CURRENT_FUEL;

	void cn_FuelStationData(string pos, int fuel)
	{
		POSITION = pos;
		CURRENT_FUEL = fuel;
	}
}

// Класс базы данных для хранения всех колонок на карте
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

	// Инициализация папок, чтение конфига настроек и базы данных остатков
	static void cn_InitializeFuelSystem()
	{
		if (cn_IsSystemInitialized) return;

		if (!FileExist(cn_ConfigDir))
		{
			MakeDirectory(cn_ConfigDir);
		}

		// Загрузка или создание файла конфигурации CN_FuelFeedConfig.json
		cn_ServerSettings = new cn_FuelConfigStructure();
		if (FileExist(cn_ConfigFile))
		{
			JsonFileLoader<cn_FuelConfigStructure>.JsonLoadFile(cn_ConfigFile, cn_ServerSettings);
		}
		else
		{
			JsonFileLoader<cn_FuelConfigStructure>.JsonSaveFile(cn_ConfigFile, cn_ServerSettings);
		}

		// Загрузка или создание файла базы данных остатков CN_FuelData.json
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

		// Запускаем сканирование карты на наличие построенных вышек из CN_MiningMachines
		cn_ScanMapAndAddBonusFuel();
	}

	// Функция сканирования карты, подсчета вышек и распределения бонуса
	static void cn_ScanMapAndAddBonusFuel()
	{
		if (cn_IsMapScannedForPumps || !cn_ServerDB || !cn_ServerSettings) return;

		if (!cn_ServerDB.STATIONS_DATA || cn_ServerDB.STATIONS_DATA.Count() <= 0)
		{
			cn_IsMapScannedForPumps = true;
			return;
		}

		// Базовый класснейм для поиска вышки (если менеджер конфигов недоступен)
		string cn_pump_classname = "CN_OilPump"; 
		
		// Интеграция с вашим менеджером конфигурации из мода CN_MiningMachines
		if (Class.CastTo(cn_pump_classname, "CN_MachineConfigManager") && CN_MachineConfigManager.GetOptions())
		{
			cn_pump_classname = CN_MachineConfigManager.GetOptions().GetOilPumpClassname(); 
		}

		// Считаем количество выстроенных игроками вышек в мире
		array<Object> cn_found_pumps = new array<Object>;
		GetGame().GetObjectsOfName(cn_pump_classname, cn_found_pumps);

		int cn_pump_count = cn_found_pumps.Count();
		if (cn_pump_count > 0)
		{
			// 1. Считаем ОБЩЕЕ количество добытого всеми вышками топлива
			float cn_total_mined_fuel = cn_pump_count * cn_ServerSettings.PUMP_FUEL_COEFFICIENT;
			
			// Количество колонок, зарегистрированных на карте в файле базы данных
			int cn_stations_count = cn_ServerDB.STATIONS_DATA.Count();

			// 2. Делим общий объем топлива, подходящий для одной колонки
			int cn_fuel_per_station = Math.Round(cn_total_mined_fuel / cn_stations_count);

			Print(string.Format("[CN_FuelSystem] Найдено вышек: %1. Всего добыто: %2 л. Доля одной АЗС: %3 л.", cn_pump_count, cn_total_mined_fuel, cn_fuel_per_station));

			// Распределяем долю по колонкам с учетом индивидуального шанса SPAWN_CHANCE_PERCENT
			for (int i = 0; i < cn_stations_count; i++)
			{
				int cn_roll = Math.RandomInt(0, 100);
				if (cn_roll < cn_ServerSettings.SPAWN_CHANCE_PERCENT)
				{
					// Шанс выпал — заливаем долю топлива в резервуар
					cn_ServerDB.STATIONS_DATA.Get(i).CURRENT_FUEL += cn_fuel_per_station;
					
					// УЧИТЫВАЕМ ВАШУ ПЕРЕМЕННУЮ: Ограничиваем максимальную емкость резервуара значением MAX_FUEL_LITRES
					if (cn_ServerDB.STATIONS_DATA.Get(i).CURRENT_FUEL > cn_ServerSettings.MAX_FUEL_LITRES)
					{
						cn_ServerDB.STATIONS_DATA.Get(i).CURRENT_FUEL = cn_ServerSettings.MAX_FUEL_LITRES;
					}
				}
				else
				{
					// Шанс не выпал — доля топлива для этой колонки бесследно испаряется
					Print(string.Format("[CN_FuelSystem] Доля топлива (%1 л.) для колонки %2 испарилась из-за шанса спавна.", cn_fuel_per_station, cn_ServerDB.STATIONS_DATA.Get(i).POSITION));
				}
			}
			
			// Фиксируем все изменения и сохранения в файл CN_FuelData.json
			cn_SaveServerDatabase();
		}

		cn_IsMapScannedForPumps = true;
	}

	// Контроль топлива при инициализации объекта в мире
	void cn_ControlFuelOnSpawn()
	{
		if (!cn_ServerSettings || !cn_ServerDB) return;

		string cn_current_pos = GetPosition().ToString();
		cn_FuelStationData cn_saved_station = cn_FindStationData(cn_current_pos);

		if (cn_saved_station)
		{
			// Если колонка найдена в базе данных, выдаем ей строго сохраненное количество литров
			cn_SetServerFuel(cn_saved_station.CURRENT_FUEL);
		}
		else
		{
			// Если колонки нет в базе данных (первый запуск мода), рассчитываем рандом из конфига
			int cn_initial_fuel = 0;
			int cn_roll = Math.RandomInt(0, 100);
			
			if (cn_roll < cn_ServerSettings.SPAWN_CHANCE_PERCENT)
			{
				cn_initial_fuel = Math.RandomIntIncluded(cn_ServerSettings.MIN_FUEL_LITRES, cn_ServerSettings.MAX_FUEL_LITRES);
			}

			// Регистрируем новую колонку
			cn_FuelStationData cn_new_station = new cn_FuelStationData(cn_current_pos, cn_initial_fuel);
			cn_ServerDB.STATIONS_DATA.Insert(cn_new_station);
			
			cn_SetServerFuel(cn_initial_fuel);
			cn_SaveServerDatabase();
		}
	}

	// Перехват расхода топлива для динамического сохранения изменений на диск
	override void DepleteFuel(float amount)
	{
		super.DepleteFuel(amount);

		if (GetGame().IsServer())
		{
			string cn_current_pos = GetPosition().ToString();
			cn_FuelStationData cn_station_data = cn_FindStationData(cn_current_pos);

			if (cn_station_data)
			{
				// Обновляем текущее значение на основе cn_GetServerFuel() из клиент-серверного аддона
				cn_station_data.CURRENT_FUEL = cn_GetServerFuel();
				cn_SaveServerDatabase();

				// Обезличенное логирование заправки в script.log
				string cn_log_msg = string.Format("[CN_FuelSystem] Произошла заправка из колонки на координатах %1. Остаток топлива: %2 л.", cn_current_pos, cn_station_data.CURRENT_FUEL);
				Print(cn_log_msg);
			}
		}
	}

	// Поиск объекта в базе данных по координатам
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

	// Безопасное сохранение базы данных на диск
	static void cn_SaveServerDatabase()
	{
		if (cn_IsSystemInitialized && cn_ServerDB)
		{
			JsonFileLoader<cn_FuelServerDatabase>.JsonSaveFile(cn_DataFile, cn_ServerDB);
		}
	}
}
