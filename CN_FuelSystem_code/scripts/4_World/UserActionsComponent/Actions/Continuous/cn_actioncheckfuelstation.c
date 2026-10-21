class cn_ActionCheckFuelStation: ActionInteractBase
{
	void cn_ActionCheckFuelStation()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_Text = "Проверить количество топлива";
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		Land_FuelStation_Feed cn_station = Land_FuelStation_Feed.Cast(target.GetObject());
		if (cn_station)
		{
			return true;
		}
		return false;
	}

	override void OnStartServer(ActionData action_data)
	{
		Land_FuelStation_Feed cn_station = Land_FuelStation_Feed.Cast(action_data.m_Target.GetObject());
		if (cn_station)
		{
			int cn_fuel = cn_station.cn_GetServerFuel();
			string cn_msg = string.Format("В баке колонки осталось: %1 литров.", cn_fuel);
			
			if (cn_fuel <= 0)
				cn_msg = "Колонка полностью пуста.";

			Param1<string> cn_params = new Param1<string>(cn_msg);
			GetGame().RPCSingleParam(action_data.m_Player, ERPCs.RPC_USER_ACTION_MESSAGE, cn_params, true, action_data.m_Player.GetIdentity());
		}
	}
}
