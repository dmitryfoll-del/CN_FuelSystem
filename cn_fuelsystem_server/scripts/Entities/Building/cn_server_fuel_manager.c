class cn_FuelConfigStructure
{
	int SPAWN_CHANCE_PERCENT;
	int MIN_FUEL_LITRES;
	int MAX_FUEL_LITRES;

	void cn_FuelConfigStructure()
	{
		SPAWN_CHANCE_PERCENT = 65;
		MIN_FUEL_LITRES = 20;
		MAX_FUEL_LITRES = 250;
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
	
	private static string cn_ConfigDir = "$profile:ColdNight_SRV_Data\\FuelSystem";
	private static string cn_ConfigFile = cn_ConfigDir + "\\CN_FuelFeedConfig.json"; // Имя вашего конфига
	private static string cn_DataFile = cn_ConfigDir + "\\CN_FuelData.json";       // Имя файла сохранения остатков

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

		// Загрузка файла конфигурации CN_FuelFeedConfig.json
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
	}

	// Контроль топлива при инициализации объекта в мире
	void cn_ControlFuelOnSpawn()
	{
		if (!cn_ServerSettings || !cn_ServerDB) return;

		string cn_current_pos = GetPosition().ToString();
		cn_FuelStationData cn_saved_station = cn_FindStationData(cn_current_pos);

		if (cn_saved_station)
		{
			// Если колонка найдена в базе данных, возвращаем ей строго сохраненное количество литров
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
