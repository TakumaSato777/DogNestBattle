# include <Siv3D.hpp> // OpenSiv3D v0.6.11
using App = SceneManager<String>;

// 食べ物の種類
constexpr size_t NumItems = 3;

// ゲームシーン
class Game : public App::Scene
{
public:

	Game(const InitData& init)
		: IScene{ init }
	{
		area.resize(7,3,0);
	}

	~Game()
	{
	}

	void update() override
	{
		{
			ClearPrint();
			Print << U"自機弾: " << playerBullets.size();
			Print << U"敵機弾: " << enemyBullets.size();
			Print << U"アイテムの個数: " << items.size();
			Print << U"timeAccumulator: " << timeAccumulator;
		}
		const double deltaTime = Scene::DeltaTime();

		// 右矢印キーが押されているかをチェック
		if (rightInput.pressed())
		{
			// 経過時間に応じて移動
			playerPos.x += moveSpeed * deltaTime;
		}

		if (leftInput.pressed())
		{
			// 経過時間に応じて移動
			playerPos.x -= moveSpeed * deltaTime;
		}

		playerPos.x = Clamp(playerPos.x, 100.0, 700.0);

		///敵制御

		// 物体が左壁に到達したら右に向かせる
		if (enemyPos.x < 100)
		{
			enemySpeed = abs(enemySpeed); // 絶対値に変更
		}
		// 物体が右壁に到達したら左に向かせる
		else if (enemyPos.x > 700)
		{
			enemySpeed = -abs(enemySpeed); // 絶対値に変更
		}

		enemyPos.x += enemySpeed * deltaTime;

		///弾発射
		// 自機ショットの発射
		// 現在の時刻を取得
		const double currentTime = Scene::Time();
		if (currentTime - lastShootTime >= cooldownTime)
		{
			if (MouseL.down()) {

				Vec2 cursorPos = Cursor::Pos();
				Vec2 direction = (cursorPos - playerPos).normalized(); // プレイヤーからカーソルへの正規化されたベクトル

				Vec2 bulletVelocity = direction * BulletSpeed;

				playerBullets << Bullet(playerPos.movedBy(0, -50), bulletVelocity);
				lastShootTime = currentTime;
			}
		}

		// 自機ショットを移動させる
		for (auto& playerBullet : playerBullets)
		{
			if (playerBullet.pos.y < 0 && playerBullet.velocity.y < 0)
			{
				playerBullet.velocity.y *= -1;
			}

			// 左右の壁にぶつかったらはね返る
			if (playerBullet.pos.x < 50 && playerBullet.velocity.x < 0 || 750 < playerBullet.pos.x && 0 < playerBullet.velocity.x)
			{
				playerBullet.velocity.x *= -1;
			}

			playerBullet.pos += playerBullet.velocity * deltaTime;
		}

		// 画面外に出た自機ショットを削除する
		playerBullets.remove_if([](const Bullet& bullet) { return (700 < bullet.pos.y); });

		// イテレータで全ての弾をたどる
		for (auto& playerBullet : playerBullets)
		{
			// 弾のあたり判定円
			const Circle BulletCircle{ playerBullet.pos, 30 };


			for (int32 i = 0; i < 3; i++) {
				for (int32 j = 0; j < 7; j++) {
					if (Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.intersects(BulletCircle)) {
						area[i][j] = 1;
					}
				}
			}
		}

		// 敵機ショットの発射
		// 現在の時刻を取得
		if (currentTime - enemylastShootTime >= enemycooldownTime){

			Vec2 enemyAim = {Random(800),Random(500.0)};

			Vec2 direction = (enemyAim - enemyPos).normalized(); // 敵からランダムな位置への正規化されたベクトル
			Vec2 enemybulletVelocity = direction * enemyBulletSpeed;
			enemyBullets << Bullet(enemyPos.movedBy(0, -50), enemybulletVelocity);
			enemylastShootTime = currentTime;
		}

		// 敵機ショットを移動させる
		for (auto& enemyBullet : enemyBullets)
		{
			if (enemyBullet.pos.y < 0 && enemyBullet.velocity.y < 0)
			{
				enemyBullet.velocity.y *= -1;
			}

			// 左右の壁にぶつかったらはね返る
			if (enemyBullet.pos.x < 50 && enemyBullet.velocity.x < 0 || 750 < enemyBullet.pos.x && 0 < enemyBullet.velocity.x)
			{
				enemyBullet.velocity.x *= -1;
			}

			enemyBullet.pos += enemyBullet.velocity * deltaTime;
		}

		// 画面外に出た敵機ショットを削除する
		enemyBullets.remove_if([](const Bullet& bullet) { return (700 < bullet.pos.y); });

		// イテレータで全ての弾をたどる
		for (auto& enemyBullet : enemyBullets)
		{
			// 弾のあたり判定円
			const Circle BulletCircle{ enemyBullet.pos, 30 };

			for (int32 i = 0; i < 3; i++) {
				for (int32 j = 0; j < 7; j++) {
					if (Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.intersects(BulletCircle)) {
						area[i][j] = 2;
					}
				}
			}
		}

		///アイテム関連
		// 経過時間の蓄積
		timeAccumulator += Scene::DeltaTime();

		while (itemSpawnTime <= timeAccumulator)
		{
			// 食べ物をランダムな場所に出現させる
			items << Item{ .pos = Vec2{ Random(50.0, 750.0), -50 },
				.type = Random<size_t>(0, NumItems - 1) };

			// 経過時間を減らす
			timeAccumulator -= itemSpawnTime;
		}

		///アイテムの移動
		const double move = (itemSpeed * Scene::DeltaTime());

		for (auto& item : items)
		{
			item.pos.y += move;
		}

		///画面外アイテム削除
		items.remove_if([](const Item& item) { return (600 < item.pos.y); });


		///クリア判定
		hantei = true;

		for (int32 i = 0; i < 3; i++) {
			for (int32 j = 0; j < 7; j++) {
				if (area[i][j] != 1) hantei = false;
			}
		}

		if (hantei) {
			changeScene(U"End");
		}

		///負けたとき
		hantei = true;

		for (int32 i = 0; i < 3; i++) {
			for (int32 j = 0; j < 7; j++) {
				if (area[i][j] != 2) hantei = false;
			}
		}

		if (hantei) {
			changeScene(U"End");
		}

	}

	void draw() const override
	{
		Scene::SetBackground(Palette::Green);
		///壁
		Rect{ 45,0,5,600 }.draw(Palette::Pink);
		Rect{ 45,0,705,5 }.draw(Palette::Pink);
		Rect{ 750,0,5,600 }.draw(Palette::Pink);

		///陣地描画

		for (int32 i = 0; i < 3; i++) {
			for (int32 j = 0; j < 7; j++) {
				if (area[i][j] == 0) {
					Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.draw(Palette::Gray).drawFrame(3, 0);
				}
				else if (area[i][j] == 1) {
					Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.draw(Palette::Blue).drawFrame(3, 0);
				}
				else if (area[i][j] == 2) {
					Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.draw(Palette::Red).drawFrame(3, 0);
				}
			}
		}

		for (const auto& item : items)
		{
			// アイテムを描く（サイズは 0.5 倍）
			itemTextures[item.type].scaled(0.5).drawAt(item.pos);
		}

		//// プレイヤーテクスチャを指定の位置で回転して描画
		enemytexture.scaled(0.7).rotated(90_deg).drawAt(enemyPos);
		dogtexture.scaled(0.7).rotated(90_deg).drawAt(playerPos);

		// 自機ショットを描画する
		for (const auto& playerBullet : playerBullets)
		{
			padtexture.scaled(0.1).drawAt(playerBullet.pos);
		}

		// 敵機ショットを描画する
		for (const auto& enemyBullet : enemyBullets)
		{
			enemypadtexture.scaled(0.1).drawAt(enemyBullet.pos);
		}

	}

private:
	Texture dogtexture{ U"🐕"_emoji };
	Texture enemytexture{ U"🐩"_emoji };
	Texture padtexture{ U"material/nikukyu_kuro.png" };
	Texture enemypadtexture{ U"material/nikukyu_pink.png" };
	Vec2 playerPos{ 400,550 };
	Vec2 enemyPos{ 400,550 };
	//移動する速度を設定
	double moveSpeed = 300.0;
	double enemySpeed = 300.0;
	const InputGroup leftInput = (KeyLeft | KeyA);
	const InputGroup rightInput = (KeyRight | KeyD);
	const InputGroup upInput = (KeyUp | KeyW);
	const InputGroup downInput = (KeyDown | KeyS);

	///弾関連
	struct Bullet {
		Vec2 pos; // 弾の位置
		Vec2 velocity; // 弾の速度

		Bullet(Vec2 _pos, Vec2 _velocity) : pos(_pos), velocity(_velocity) {}
	};
	// 自機ショット
	Array<Bullet> playerBullets;
	// 自機ショットのスピード
	double BulletSpeed = 500.0;
	// 弾のクールダウン時間（秒単位）を設定
	double cooldownTime = 2.0;
	double lastShootTime = 0.0;

	///敵機ショット
	Array<Bullet> enemyBullets;
	double enemyBulletSpeed = 500.0;
	double enemycooldownTime = 3.0;
	double enemylastShootTime = 0.0;

	///アイテム関連
	struct Item
	{
		Vec2 pos;

		size_t type = 0;
	};
	Array<Item> items;
	// アイテムのテクスチャ
	const std::array<Texture, NumItems> itemTextures =
	{
		Texture{ Emoji{ U"🦴" }},
		Texture{ Emoji{ U"🍖" }},
		Texture{ Emoji{ U"😻" }},
	};
	// アイテムが毎秒何ピクセルの速さで落下するか
	double itemSpeed = 200.0;
	// 何秒ごとにアイテムが出現するか
	double itemSpawnTime = 5.0;
	// 前回の食べ物の出現から何秒経過したか
	double timeAccumulator = 0.0;

	///陣地関連
	/*Array<int32> area;*/
	Grid<int32> area;

	bool hantei = true;
};

// エンドシーン
class End : public App::Scene
{
public:

	// コンストラクタ（必ず実装）
	End(const InitData& init)
		: IScene{ init }
	{

	}

	~End()
	{
	}


	// 更新関数（オプション）
	void update() override
	{
		if (MouseL.down())
		{
			// Gameシーンに遷移
			changeScene(U"Game");
		}
	}

	// 描画関数（オプション）
	void draw() const override
	{
		Scene::SetBackground(ColorF{ 0.3, 0.4, 0.5 });

		font(U"制圧完了!").draw(200, 200);

	}

private:
	/// 基本サイズ 50 のフォントを作成
	Font font = Font(100);
};

void Main()
{
	// シーンマネージャーを作成
	App manager;
	manager.add<Game>(U"Game");
	manager.add<End>(U"End");

	// "Game" シーンから開始
	manager.init(U"Game");

	while (System::Update())
	{
		if (not manager.update())
		{
			break;
		}
	}
}
