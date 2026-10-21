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

modded class Land_FuelStation_Feed
{
	private static ref cn_FuelConfigStructure cn_ServerSettings;
	private static bool cn_IsConfigLoaded = false;

	override void EEInit()
	{
		super.EEInit();

		if (GetGame().IsServer())
		{
			cn_LoadServerConfig();
			cn_ApplyRandomFuel();
		}
	}

	static void cn_LoadServerConfig()
	{
		if (cn_IsConfigLoaded) return;

		// Установка кастомного пути: profiles/ColdNight_SRV_Data/FuelSystem
		string cn_dir = "$profile:ColdNight_SRV_Data\\FuelSystem";
		string cn_file = cn_dir + "\\CN_Config.json";

		// Проверяем и создаем всю вложенную структуру папок, если она отсутствует
		if (!FileExist(cn_dir))
		{
			MakeDirectory(cn_dir);
		}

		cn_ServerSettings = new cn_FuelConfigStructure();

		if (FileExist(cn_file))
		{
			JsonFileLoader<cn_FuelConfigStructure>.JsonLoadFile(cn_file, cn_ServerSettings);
		}
		else
		{
			JsonFileLoader<cn_FuelConfigStructure>.JsonSaveFile(cn_file, cn_ServerSettings);
		}

		cn_IsConfigLoaded = true;
	}

	void cn_ApplyRandomFuel()
	{
		if (!cn_ServerSettings) return;

		int cn_roll = Math.RandomInt(0, 100);
		if (cn_roll < cn_ServerSettings.SPAWN_CHANCE_PERCENT)
		{
			int cn_generated_litres = Math.RandomIntIncluded(cn_ServerSettings.MIN_FUEL_LITRES, cn_ServerSettings.MAX_FUEL_LITRES);
			cn_SetServerFuel(cn_generated_litres);
		}
		else
		{
			cn_SetServerFuel(0);
		}
	}
}
