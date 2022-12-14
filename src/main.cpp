#define OLC_PGE_APPLICATION
//#define OLC_GFX_OPENGL33
#include "olcPixelGameEngine.h"
//#include "Extensions/olcPGEX_Network.h"
//#include "network.h" Not using since I don't know everything about it
#include "resourcemanager.h"
#include "soundsystem.h"
#include "weather.h"

struct Player
{
	olc::vf2d vPos;

	float health;
};

class App : public olc::PixelGameEngine
{
	olc::Sprite* m_pSprite;
	olc::Sprite* m_pThunderSprite;
public:

	enum AI_STATE
	{
		AI_WAIT,
		AI_ATTACK
		//AI_Something...
	} nAIState, nNextAIState;

	enum GAME_STATE
	{
		GAME_RESET, // Stage when game is booted up ig
		GAME_PREPARE, // Stage starting each time when game round is ended
		GAME_PREPARING, // Stage for preparing stuff like deciding where to spawn player, start timer, idk
		GAME_ROUND_START, // Stage indicating round is started
		GAME_ROUND_END, // Stage indicating round end, like when you survive
		GAME_OVER // Stage indicating that player finished all rounds or died
	} nGameState, nNextGameState;

	enum SOUND_TYPE
	{
		SOUND_TYPE_RAIN,
		SOUND_TYPE_THUNDER,
		SOUND_TYPE_ROUND_THEME,
		SOUND_TYPE_MAX
	};

	App() :
		m_pSprite(nullptr),
		m_pThunderSprite(nullptr)
	{
		sAppName = "Survive The Thunder";
	}
private:

	bool bUserControlEnabled = true;
	bool bAIEnabled = false;

	cSound* m_pSounds[SOUND_TYPE_MAX];

	cThunder* m_pThunder;

	void LoadSounds()
	{
		m_pSounds[SOUND_TYPE_RAIN] = g_pSoundSystem->CreateSound("res/sounds/rain1.wav");
		m_pSounds[SOUND_TYPE_THUNDER] = g_pSoundSystem->CreateSound("res/sounds/thunder2.wav");
		m_pSounds[SOUND_TYPE_ROUND_THEME] = g_pSoundSystem->CreateSound("res/sounds/round_theme.wav");

		//m_pSounds[SOUND_TYPE_RAIN]->Play(true);
	}

	void SoundOnSwitchState()
	{
		cSound* pRain = m_pSounds[SOUND_TYPE_RAIN];
		cSound* pRoundTheme = m_pSounds[SOUND_TYPE_ROUND_THEME];
		cSound* pThunder = m_pSounds[SOUND_TYPE_THUNDER];

		switch (nGameState)
		{
		case GAME_RESET:
			if (pRoundTheme && !pRoundTheme->IsPlaying())
				pRoundTheme->Play(true);

			break;

		case GAME_PREPARE:
			break;

		case GAME_PREPARING:
			if (pRoundTheme && !pRoundTheme->IsPlaying())
				pRoundTheme->Play(true);
			break;

		case GAME_ROUND_START:
			if (pRoundTheme && pRoundTheme->IsPlaying())
				pRoundTheme->Stop();

			if (pRain && !pRain->IsPlaying())
				pRain->Play(true);

			break;

		case GAME_ROUND_END:
			if (pRoundTheme && pRoundTheme->IsPlaying())
				pRoundTheme->Stop();

			if (pRain && pRain->IsPlaying())
				pRain->Stop();
			
			if (pThunder && pThunder->IsPlaying())
				pThunder->Stop();

			break;

		case GAME_OVER:
			if (pRain && pRain->IsPlaying())
				pRain->Stop();

			if (pThunder && pThunder->IsPlaying())
				pThunder->Stop();

			break;

		}

		switch (nAIState)
		{
		case AI_ATTACK:
			if (pThunder && pThunder->IsPlaying())
				pThunder->Stop();

			if(pThunder && !pThunder->IsPlaying())
				pThunder->Play(true);
			break;
		}
	}

public:
	bool OnUserCreate() override
	{
		// Resource manager init
		g_pResourceManager = new cResourceManager();
		g_pResourceManager->Init();

		// Sound system init
		g_pSoundSystem = new cSoundSystem();
		g_pSoundSystem->Init();

		LoadSounds();

		m_pThunder = new cThunder();

		nAIState = AI_WAIT;
		nNextAIState = AI_WAIT;
		nGameState = GAME_RESET;
		nNextGameState = GAME_RESET;

		/*olc::ResourcePack* pResourcePack = new olc::ResourcePack();
		pResourcePack->AddFile("res/test.bmp");

		pResourcePack->SavePack("datapc_main.respack", "easy peasy lemon squeezy");*/

		m_pSprite = g_pResourceManager->LoadSprite("res/player_new.png");

		m_pThunderSprite = g_pResourceManager->LoadSprite("res/new_thunder.png");

		g_pResourceManager->SaveResourcePack();

		return true;
	}

	bool OnUserDestroy() override
	{
		delete m_pThunder;

		g_pSoundSystem->Shutdown();
		delete g_pSoundSystem;

		g_pResourceManager->Shutdown();
		delete g_pResourceManager;

		return true;
	}

	float fTargetFrameTime = 1.0f / 100.0f; // Virtual FPS of 100fps
	float fAccumulatedTime = 0.0f;

	bool OnUserUpdate(float fElapsedTime) override
	{
		//fAccumulatedTime += fElapsedTime;
		//if (fAccumulatedTime >= fTargetFrameTime)
		//{
		//	fAccumulatedTime -= fTargetFrameTime;
		//	fElapsedTime = fTargetFrameTime;
		//}
		//else
		//	return true; // Don't do anything this frame

		return GameUpdate(fElapsedTime);
	}

	bool GameUpdate(float fElapsedTime)
	{
		Clear(olc::DARK_GREEN);

		switch (nGameState)
		{
		case GAME_RESET:
			prepareTimer = 0;
			roundTimer = 0;
			nNumRounds = 0;
			nAIAttackInterval = 5;
			player.health = 100.0f;
			bGameOver = false;
			//DrawString({ 3,0 }, "Current state: GAME_RESET");
			bUserControlEnabled = false;
			bAIEnabled = false;
			nNextGameState = GAME_PREPARE;
			break;
		case GAME_PREPARE:
			//DrawString({ 3,0 }, "Current state: GAME_PREPARE");
			// temporary enabling controls
			//bUserControlEnabled = true;
			bAbleToDrawPlayer = false;
			player.vPos = { rand() % ScreenWidth() / 1.5f, rand() % ScreenHeight() / 1.5f };
			nNextGameState = GAME_PREPARING;
			break;
		case GAME_PREPARING:
			//DrawString({ 3,0 }, "Current state: GAME_PREPARING");
			prepareTimer += fElapsedTime;
			if (prepareTimer <= 4.0f)
			{
				DrawString({ (ScreenWidth() / 3), (ScreenHeight() / 2) - 8 }, "Prepare for round!");
				DrawString({ (ScreenWidth() / 2) - 8, (ScreenHeight() / 2) + 10 }, std::to_string((int)prepareTimer));
			}
			else
			{
				nNextGameState = GAME_ROUND_START;
			};


			break;
		case GAME_ROUND_START:
			//DrawString({ 3,0 }, "Current state: GAME_ROUND_START");
			// Draw player's health
			DrawString({ 0,ScreenHeight() - 10 }, "Player Health: ");
			DrawString({ sizeof("Player Health: ") * 8,ScreenHeight() - 10 }, std::to_string((int)player.health));
			bAIEnabled = true;
			bUserControlEnabled = true;
			// Enable player draw
			bAbleToDrawPlayer = true;
			roundTimer += fElapsedTime;



			if (roundTimer < 11.0f)
			{
				DrawString({ (ScreenWidth() / 2) - 50, (ScreenHeight() / 2) - 8 }, "Round time: ");
				DrawString({ ((ScreenWidth() / 2) + ((int)sizeof("Round time: ") * 8)) - 50, (ScreenHeight() / 2) - 8 }, std::to_string((int)roundTimer));
			}
			else
			{
				nNextGameState = GAME_ROUND_END;
			}

			if (player.health <= 0)
			{
				nNextGameState = GAME_OVER;
			}

			break;
		case GAME_ROUND_END:
			//DrawString({ 3,0 }, "Current state: GAME_ROUND_END");
			bAIEnabled = false;
			bUserControlEnabled = false;
			nNumRounds++;
			prepareTimer = 0;
			roundTimer = 0;
			// Lower AI attack interval each round;
			nAIAttackInterval -= 1.5f;
			if (nNumRounds == 3)
			{
				nNextGameState = GAME_OVER;
				break;
			}
			else
			{
				DrawString({ (ScreenWidth() / 2) - 8, (ScreenHeight() / 2) - 8 }, "Round survived!");
				nNextGameState = GAME_PREPARE;
				break;
			}

			break;
		case GAME_OVER:
			// Disable controls and AI, display text and suggest restarting the game;
			//DrawString({ 3,0 }, "Current state: GAME_OVER");
			bAIEnabled = false;
			bUserControlEnabled = false;
			bGameOver = true;
			bAbleToDrawLighting = false;

			break;
		}

		SoundOnSwitchState();

		if (bAIEnabled)
		{

			switch (nAIState)
			{
			case AI_WAIT:
				// Prepare for attack, wait for interval

				//DrawString({ ScreenWidth() - ((int)sizeof("AI state: AI_WAIT") * 8), 0 }, "AI state: AI_WAIT");
				fAIAttackTimer += fElapsedTime;
				if (nAIAttackInterval < fAIAttackTimer)
				{
					vLastPos = player.vPos;
					nNextAIState = AI_ATTACK;
				}
				break;
			case AI_ATTACK:
				// Attack the player, get last player pos, damage and reduce health
				//DrawString({ ScreenWidth() - ((int)sizeof("AI state: AI_ATTACK") * 8), 0 }, "AI state: AI_ATTACK");
				bAbleToDrawLighting = true;
				//FillRect(vLastPos, { 20,20 }, olc::RED);
				// Check if last grabbed pos is equal to current pos and damage the player
				if (vLastPos == player.vPos)
				{
					player.health -= 10.0f;

				}
				// Reset attack timer
				fAIAttackTimer = 0;
				nNextAIState = AI_WAIT;
				break;
			}
		}

		if (bUserControlEnabled)
		{
			if (GetKey(olc::Key::W).bHeld) fElapsedTime * (player.vPos += {0.0f, -5.0f});
			if (GetKey(olc::Key::S).bHeld) fElapsedTime * (player.vPos += {0.0f, 5.0f});
			if (GetKey(olc::Key::A).bHeld) fElapsedTime * (player.vPos += {-5.0f, 0.0f});
			if (GetKey(olc::Key::D).bHeld) fElapsedTime * (player.vPos += {5.0f, 0.0f});
		}

		if (player.vPos.x < 0) player.vPos.x = 0;
		if (player.vPos.y < 0) player.vPos.y = 0;
		if (player.vPos.x > (ScreenWidth() - 20)) player.vPos.x = ScreenWidth() - 20;
		if (player.vPos.y > (ScreenHeight() - 20)) player.vPos.y = ScreenHeight() - 20;

		// Draw player
		if (bAbleToDrawPlayer)
		{
			//FillRect(player.vPos, { 20, 20 });
			if (m_pSprite)
			{
				SetPixelMode(olc::Pixel::ALPHA);

				DrawSprite(player.vPos, m_pSprite, 2);

				SetPixelMode(olc::Pixel::NORMAL);
			}
			//m_pThunder->Render(this);
		}

		// Draw Lightning
		if (bAbleToDrawLighting)
		{
			SetPixelMode(olc::Pixel::ALPHA);

			olc::vd2d translatedThunderPos = vLastPos;
			//translatedThunderPos.y -= 20;

			DrawSprite(translatedThunderPos, m_pThunderSprite, 2);

			SetPixelMode(olc::Pixel::NORMAL);

			fDrawTime += fElapsedTime;
			if (fDrawTime > 1.0f)
			{
				bAbleToDrawLighting = false;
				fDrawTime = 0;
			}
		}
		// Test output
		if (bGameOver)
		{
			Clear(olc::DARK_RED);
			DrawString({ ScreenWidth() / 2 - 50, ScreenHeight() / 2 }, "GAME OVER", olc::BLACK);
			DrawString({ ScreenWidth() / 2 - 100, ScreenHeight() / 2 + 15 }, "Press SPACE to restart!", olc::BLACK);
			if (GetKey(olc::Key::SPACE).bReleased) nNextGameState = GAME_RESET;
		}
		/*if (m_pSprite)
		{
			DrawSprite(olc::vi2d(0, 0), m_pSprite);
		}*/
		//if (GetKey(olc::Key::ESCAPE).bReleased) return false;
		nGameState = nNextGameState;
		nAIState = nNextAIState;

		return true;
	}

private:
	bool bGameOver = false;
	bool bAbleToDrawPlayer = false;
	bool bAbleToDrawLighting = false;
	float prepareTimer = 1;
	float roundTimer = 1;
	float fAIAttackTimer = 0;
	float fDrawTime = 0;
	int nAIAttackInterval = 0;
	int nNumRounds;
	olc::vf2d vLastPos = { 0,0 };
	Player player;

};


int main()
{
	App application;
	if (application.Construct(800, 600, 1, 1,false))
		application.Start();

	return 0;
}