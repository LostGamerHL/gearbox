/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define WEAPON_SAW 21

enum m249_e
{
	M249_SLOWIDLE = 0,
	M249_IDLE2,
	M249_LAUNCH,
	M249_RELOAD1,
	M249_HOLSTER,
	M249_DEPLOY,
	M249_SHOOT1,
	M249_SHOOT2,
	M249_SHOOT3,
};

class CM249 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;

	virtual BOOL UseDecrement( void )
	{ 
		return FALSE;
	}

private:
	//unsigned short m_usM249;
};



LINK_ENTITY_TO_CLASS( weapon_m249, CM249 )

void CM249::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_m249" ); // hack to allow for old names
	Precache();
	SET_MODEL( ENT( pev ), "models/w_saw.mdl" );
	m_iId = WEAPON_SAW;

	m_iDefaultAmmo = 100;

	FallInit();// get ready to fall down.
}

void CM249::Precache(void)
{
	PRECACHE_MODEL("models/v_saw.mdl");
	PRECACHE_MODEL("models/w_saw.mdl");
	PRECACHE_MODEL("models/p_saw.mdl");

	m_iShell = PRECACHE_MODEL("models/saw_shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_saw_clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/saw_fire1.wav");

	PRECACHE_SOUND("weapons/saw_reload.wav");
	PRECACHE_SOUND("weapons/saw_reload2.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	//m_usM249 = PRECACHE_EVENT(1, "events/saw.sc");
}


int CM249::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = 300;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 100;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SAW;
	p->iWeight = 15;

	return 1;
}

int CM249::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CM249::Deploy()
{
	return DefaultDeploy( "models/v_saw.mdl", "models/p_saw.mdl", M249_DEPLOY, "saw" );
}

void CM249::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if( m_iClip <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( 1.0f, 1.5f );
	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT( -0.5f, -0.2f );

	// Add inverse impulse, but only if we are on the ground.
	// This is mainly to avoid too-high air velocity.
	if (m_pPlayer->pev->flags & FL_ONGROUND)
	{
		float flZVel = m_pPlayer->pev->velocity.z;

		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * (40 + (RANDOM_LONG(1, 2) * 2));

		// Add backward velocity
		m_pPlayer->pev->velocity.z = flZVel;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;
#ifdef CLIENT_DLL
	if( !bIsMultiplayer() )
#else
	if( !g_pGameRules->IsMultiplayer() )
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer( 4, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_9MM, 0, 3, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer( 4, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_9MM, 0, 3, m_pPlayer->pev, m_pPlayer->random_seed );
	}

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	pev->effects = EF_MUZZLEFLASH;
	//PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usM249, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	
		SendWeaponAnim(M249_SHOOT1);
		break;
	case 1:	
		SendWeaponAnim(M249_SHOOT2);
		break;
	case 2:
		SendWeaponAnim(M249_SHOOT3);
		break;
	}

	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "weapons/saw_fire1.wav", 1, ATTN_NORM );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = gpGlobals->time + UTIL_WeaponTimeBase() + 0.1;

	if( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = gpGlobals->time +  UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = gpGlobals->time +  UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CM249::Reload( void )
{
	if( m_pPlayer->ammo_556 <= 0 )
		return;

	DefaultReload( 100, M249_RELOAD1, 3.55);
}

void CM249::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = M249_SLOWIDLE;	
		break;
	default:
	case 1:
		iAnim = M249_IDLE2;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

class CM249Ammo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/w_ARgrenade.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_saw_clip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = ( pOther->GiveAmmo( 100, "556", 300 ) != -1 );

		if( bResult )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_556, CM249Ammo )
