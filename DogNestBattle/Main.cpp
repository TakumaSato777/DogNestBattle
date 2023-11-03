# include <Siv3D.hpp> // OpenSiv3D v0.6.11

struct GameData
{
	int32 hp = 3;
	int32 GameCount = 0;
	///陣地関連
	Grid<int32> area;

	///家関連
	int32 houseX = 0;
	int32 houseY = 0;
	int32 houseCount = 0;
	// 自機ショットのスピード
	double BulletSpeed = 500.0;
	// 弾のクールダウン時間（秒単位）を設定
	double cooldownTime = 2.0;

	///敵機ステータス
	double enemyBulletSpeed = 800.0;
	double enemycooldownTime = 2.0;

	///エンド判定
	int32 end = 0;
};

using App = SceneManager<String,GameData>;

// アイテムの種類
constexpr size_t NumItems = 2;

// スタートシーン
class Start : public App::Scene
{
public:

	// コンストラクタ（必ず実装）
	Start(const InitData& init)
		: IScene{ init }
	{
		Window::Resize(900, 600);
	}

	~Start()
	{
		
	}


	// 更新関数（オプション）
	void update() override
	{
		if (SimpleGUI::Button(U"スタート", Vec2{ 300, 500 },200))///ボタン
		{
			changeScene(U"Game",0.1s);
		}

		// 経過時間の蓄積
		cattimeAccumulator += Scene::DeltaTime();

		while (catSpawnTime <= cattimeAccumulator)
		{
			if (RandomBool()) {
				// アイテムを左に出現させる
				cats << Cat{ .pos = Vec2{ -50, Random(50,550)},.type = 0};
			}
			else {
				// アイテムを右に出現させる
				cats << Cat{ .pos = Vec2{ 850, Random(50,550)},.type = 1};
			}

			/*catSpeed += 50;*/
			// 経過時間を減らす
			cattimeAccumulator -= catSpawnTime;
		}

		///猫の移動
		const double catmove = (catSpeed * Scene::DeltaTime());

		for (auto& cat : cats)
		{
			if (cat.type == 0)cat.pos.x += catmove;
			else cat.pos.x -= catmove;
		}

		///画面外の猫削除
		cats.remove_if([](const Cat& cat) { return (900 < cat.pos.x); });
		cats.remove_if([](const Cat& cat) { return (-100 > cat.pos.x); });


	}

	// 描画関数（オプション）
	void draw() const override
	{
		Scene::SetBackground(ColorF{ 0.3, 0.4, 0.5 });
		/*yuyaketexture.draw();*/

		// 猫を描画する
		for (const auto& cat : cats)
		{
			if (cat.type == 0) cattexture.mirrored().scaled(0.5).drawAt(cat.pos);
			else cattexture.scaled(0.5).drawAt(cat.pos);
		}

		font(U"Dog Nest Battles").draw(200, 100);
		font2(U"feat. Neko").draw(290, 200);
	}

private:
	/// 基本サイズ 100 のフォントを作成
	Font font = Font (50,U"material/LightNovelPOPv2.otf" );
	Font font2 = Font (40, U"material/LightNovelPOPv2.otf");

	Texture yuyaketexture{ Resource(U"material/yuyake.jpg") };

	///猫
	struct Cat
	{
		Vec2 pos;

		size_t type = 0;
	};
	Array<Cat> cats;

	Texture cattexture{ U"🐈"_emoji };

	// 猫が毎秒何ピクセルの速さで移動するか
	double catSpeed = 800.0;
	// 前回の猫の出現から何秒経過したか
	double cattimeAccumulator = 0.0;
	// 何秒ごとに猫が出現するか
	double catSpawnTime = 5.0;
	// 前回の食べ物の出現から何秒経過したか
	double catAccumulator = 0.0;
};

// ゲームシーン
class Game : public App::Scene
{
public:

	Game(const InitData& init)
		: IScene{ init }
	{
		if (getData().GameCount == 0) {
			getData().area.resize(7, 3, 0);
			getData().houseX = Random(6);
			getData().houseY = Random(2);
		}
		getData().GameCount++;

		Window::Resize(900, 600);
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
			Print << U"アイテムのクールダウン: " << timeAccumulator;
			Print << U"クールダウン: " << getData().cooldownTime;
			Print << U"弾速度: " << getData().BulletSpeed;
			Print << U"家にぶつかった回数: " << getData().houseCount;
			Print << U"HP: " << getData().hp;
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
		if (currentTime - lastShootTime >= getData().cooldownTime)
		{
			if (MouseL.down()) {

				Vec2 cursorPos = Cursor::Pos();
				Vec2 direction = (cursorPos - playerPos).normalized(); // プレイヤーからカーソルへの正規化されたベクトル

				Vec2 bulletVelocity = direction * getData().BulletSpeed;

				playerBullets << Bullet(playerPos.movedBy(0, -50), bulletVelocity);
				lastShootTime = currentTime;
			}
		}

		// 自機ショットを移動させる
		for (auto& playerBullet : playerBullets)
		{
			// 弾のあたり判定円
			const Circle BulletCircle{ playerBullet.pos, 30 };

			if (playerBullet.pos.y < 0 && playerBullet.velocity.y < 0)
			{
				playerBullet.velocity.y *= -1;
			}

			// 左右の壁にぶつかったらはね返る
			if (playerBullet.pos.x < 50 && playerBullet.velocity.x < 0 || 750 < playerBullet.pos.x && 0 < playerBullet.velocity.x)
			{
				playerBullet.velocity.x *= -1;
			}

			///家関連
			if (Quad{ Vec2{50 + 100 * getData().houseX,5 + 100 * getData().houseY},Vec2{55 + 100 * getData().houseX,10 + 100 * getData().houseY},Vec2{145 + 100 * getData().houseX,10 + 100 * getData().houseY},Vec2{150 + 100 * getData().houseX,5 + 100 * getData().houseY} }.intersects(BulletCircle)) {///上辺との接触
				playerBullet.velocity.y *= -1;
				getData().houseCount++;
			}
			if (Quad{ Vec2{50 + 100 * getData().houseX,5 + 100 * getData().houseY},Vec2{55 + 100 * getData().houseX,10 + 100 * getData().houseY},Vec2{55 + 100 * getData().houseX,100 + 100 * getData().houseY},Vec2{50 + 100 * getData().houseX,105 + 100 * getData().houseY} }.intersects(BulletCircle)) {///左辺との接触
				playerBullet.velocity.x *= -1;
				getData().houseCount++;
			}
			if (Quad{ Vec2{55 + 100 * getData().houseX,100 + 100 * getData().houseY},Vec2{50 + 100 * getData().houseX,105 + 100 * getData().houseY},Vec2{150 + 100 * getData().houseX,105 + 100 * getData().houseY},Vec2{145 + 100 * getData().houseX,100 + 100 * getData().houseY} }.intersects(BulletCircle)) {///下辺との接触
				playerBullet.velocity.y *= -1;
				getData().houseCount++;
			}
			if (Quad{ Vec2{145 + 100 * getData().houseX,10 + 100 * getData().houseY},Vec2{150 + 100 * getData().houseX,5 + 100 * getData().houseY},Vec2{150 + 100 * getData().houseX,105 + 100 * getData().houseY},Vec2{145 + 100 * getData().houseX,100 + 100 * getData().houseY} }.intersects(BulletCircle)) {///右辺との接触
				playerBullet.velocity.x *= -1;
				getData().houseCount++;
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
					if (!(j == getData().houseX && i == getData().houseY)) {
						if (Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.intersects(BulletCircle)) {
							getData().area[i][j] = 1;
						}
					}
				}
			}
		}

		// 敵機ショットの発射
		// 現在の時刻を取得
		if (currentTime - enemylastShootTime >= getData().enemycooldownTime){

			Vec2 enemyAim = {Random(800),Random(500.0)};

			Vec2 direction = (enemyAim - enemyPos).normalized(); // 敵からランダムな位置への正規化されたベクトル
			Vec2 enemybulletVelocity = direction * getData().enemyBulletSpeed;
			enemyBullets << Bullet(enemyPos.movedBy(0, -50), enemybulletVelocity);
			enemylastShootTime = currentTime;
		}

		// 敵機ショットを移動させる
		for (auto& enemyBullet : enemyBullets)
		{
			// 弾のあたり判定円
			const Circle BulletCircle{ enemyBullet.pos, 30 };

			if (enemyBullet.pos.y < 0 && enemyBullet.velocity.y < 0)
			{
				enemyBullet.velocity.y *= -1;
			}

			// 左右の壁にぶつかったらはね返る
			if (enemyBullet.pos.x < 50 && enemyBullet.velocity.x < 0 || 750 < enemyBullet.pos.x && 0 < enemyBullet.velocity.x)
			{
				enemyBullet.velocity.x *= -1;
			}

			///家関連
			if (Quad{ Vec2{50 + 100 * getData().houseX,5 + 100 * getData().houseY},Vec2{55 + 100 * getData().houseX,10 + 100 * getData().houseY},Vec2{145 + 100 * getData().houseX,10 + 100 * getData().houseY},Vec2{150 + 100 * getData().houseX,5 + 100 * getData().houseY}}.intersects(BulletCircle)) {///上辺との接触
				enemyBullet.velocity.y *= -1;
			}
			if (Quad{ Vec2{50 + 100 * getData().houseX,5 + 100 * getData().houseY},Vec2{55 + 100 * getData().houseX,10 + 100 * getData().houseY},Vec2{55 + 100 * getData().houseX,100 + 100 * getData().houseY},Vec2{50 + 100 * getData().houseX,105 + 100 * getData().houseY} }.intersects(BulletCircle)) {///左辺との接触
				enemyBullet.velocity.x *= -1;
			}
			if (Quad{ Vec2{55 + 100 * getData().houseX,100 + 100 * getData().houseY},Vec2{50 + 100 * getData().houseX,105 + 100 * getData().houseY},Vec2{150 + 100 * getData().houseX,105 + 100 * getData().houseY},Vec2{145 + 100 * getData().houseX,100 + 100 * getData().houseY} }.intersects(BulletCircle)) {///下辺との接触
				enemyBullet.velocity.y *= -1;
			}
			if (Quad{ Vec2{145 + 100 * getData().houseX,10 + 100 * getData().houseY},Vec2{150 + 100 * getData().houseX,5 + 100 * getData().houseY},Vec2{150 + 100 * getData().houseX,105 + 100 * getData().houseY},Vec2{145 + 100 * getData().houseX,100 + 100 * getData().houseY}}.intersects(BulletCircle)) {///右辺との接触
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
					if (!(j == getData().houseX && i == getData().houseY)) {
						if (Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.intersects(BulletCircle)) {
							getData().area[i][j] = 2;
						}
					}
				}
			}
		}

		///敵機ステータス更新
		if (currentTime - statuslastupdateTime >= statuscooldownTime) {

			getData().enemyBulletSpeed += 100.0;
			getData().enemycooldownTime -= 0.2;
			if (getData().enemycooldownTime < 0.2) getData().enemycooldownTime = 0.1;
			statuslastupdateTime = currentTime;
		}

		///アイテム関連
		// 経過時間の蓄積
		timeAccumulator += Scene::DeltaTime();

		while (itemSpawnTime <= timeAccumulator)
		{
			// アイテムをランダムな場所に出現させる
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

		///アイテムの当たり判定
		Circle playerCircle{ playerPos, 60 };
		for (auto it = items.begin(); it != items.end(); ) {
			Circle itemCircle{ it->pos, 30 };
			if (playerCircle.intersects(itemCircle)) {
				if (it->type == 0) {
					getData().BulletSpeed += 100.0;
				}
				else {
					getData().cooldownTime -= 0.5;
					if (getData().cooldownTime < 0.5) getData().cooldownTime = 0.1;
				}
				// 削除する要素
				it = items.erase(it);
			}
			else {
				++it;
			}
		}

		///画面外アイテム削除
		items.remove_if([](const Item& item) { return (600 < item.pos.y); });

		///猫関連
		// 経過時間の蓄積
		cattimeAccumulator += Scene::DeltaTime();

		while (catSpawnTime <= cattimeAccumulator)
		{
			if (RandomBool()) {
				// アイテムを左に出現させる
				cats << Cat{ .pos = Vec2{ -50, 550 },.type = 0 };
			}
			else {
				// アイテムを右に出現させる
				cats << Cat{ .pos = Vec2{ 850, 550 },.type = 1 };
			}

			catSpeed += 50;
			// 経過時間を減らす
			cattimeAccumulator -= catSpawnTime;
		}

		///猫の移動
		const double catmove = (catSpeed * Scene::DeltaTime());

		for (auto& cat : cats)
		{
			if (cat.type == 0)cat.pos.x += catmove;
			else cat.pos.x -= catmove;
		}

		/////弾と猫の当たり判定
		for (auto it = cats.begin(); it != cats.end(); ) {
			Circle catCircle{ it->pos, 30 };
			bool intersectionFound = false;

			for (auto& playerBullet : playerBullets) {
				const Circle BulletCircle{ playerBullet.pos, 30 };
				if (BulletCircle.intersects(catCircle)) {
					// 削除する要素の位置を記録
					it = cats.erase(it);
					intersectionFound = true;
					break;
				}
			}
			// もし交差が見つからなかった場合、イテレータを更新
			if (!intersectionFound) {
				++it;
			}
		}

		/////プレイヤーと猫の当たり判定
		for (auto it = cats.begin(); it != cats.end(); ) {
			Circle catCircle{ it->pos, 30 };
			bool intersectionFound = false;
			/*Circle playerCircle{ playerPos, 30 };*/

			if (playerCircle.intersects(catCircle)) {
				// 削除する要素の位置を記録
				it = cats.erase(it);
				intersectionFound = true;
				changeScene(U"Neko");
				break;
			}
			// もし交差が見つからなかった場合、イテレータを更新
			if (!intersectionFound) {
				++it;
			}
		}

		///画面外の猫削除
		cats.remove_if([](const Cat& cat) { return (900 < cat.pos.x); });
		cats.remove_if([](const Cat& cat) { return (-100 > cat.pos.x); });

		///終了判定///
		///クリア時
		hantei = true;

		for (int32 i = 0; i < 3; i++) {
			for (int32 j = 0; j < 7; j++) {
				if (!(j == getData().houseX && i == getData().houseY)) {
					if (getData().area[i][j] != 1) hantei = false;
				}
			}
		}

		if (hantei) {
			getData().end = 1;
			changeScene(U"End");
		}

		///陣地を全て取られて負けた時
		hantei = true;

		for (int32 i = 0; i < 3; i++) {
			for (int32 j = 0; j < 7; j++) {
				if (!(j == getData().houseX && i == getData().houseY)) {
					if (getData().area[i][j] != 2) hantei = false;
				}
			}
		}

		if (hantei) {
			getData().end = 2;
			changeScene(U"End");
		}

		///HPが0になって負けた時
		if (getData().hp == 0) {
			getData().end = 3;
			changeScene(U"End");
		}

		///家にヒットした回数が50以上なら
		if(getData().houseCount >= 50) {
			getData().end = 4;
			changeScene(U"End");
		}

		///背景関連
		// 経過時間の蓄積
		tumbletimeAccumulator += Scene::DeltaTime();

		while (tumbleSpawnTime <= tumbletimeAccumulator)
		{
			if (RandomBool()) {
				// アイテムを左に出現させる
				tumbles << Tumble{ .pos = Vec2{ -50, Random(50, 550) },.type = 0,.angle = 0 };
			}
			else {
				// アイテムを右に出現させる
				tumbles << Tumble{ .pos = Vec2{ 850, Random(50, 550) },.type = 1 };
			}

			// 経過時間を減らす
			tumbletimeAccumulator -= tumbleSpawnTime;
		}

		///tumbleの移動
		const double tumblemove = (tumbleSpeed * Scene::DeltaTime());

		for (auto& tumble : tumbles)
		{
			if (tumble.type == 0) {
				tumble.pos.x += tumblemove;
				tumble.angle += 5.0 * Scene::DeltaTime();
			}
			else {
				tumble.pos.x -= tumblemove;
				tumble.angle -= 5.0 * Scene::DeltaTime();
			}
		}

		///画面外の猫削除
		tumbles.remove_if([](const Tumble& tumble) { return (900 < tumble.pos.x); });
		tumbles.remove_if([](const Tumble& tumble) { return (-100 > tumble.pos.x); });


		///しっぽ時間経過判定
		shippotimeAccumulator += Scene::DeltaTime();

		while (shippoTime <= shippotimeAccumulator)
		{
			if (shippohantei) {
				shippohantei = false;
			}
			else {
				shippohantei = true;
			}

			// 経過時間を減らす
			shippotimeAccumulator -= shippoTime;
		}

	}

	void draw() const override
	{
		///背景
		/*Scene::SetBackground(Palette::Green);*/
		yuyaketexture.draw();
		///タンブル描画する
		for (const auto& tumble : tumbles)
		{
			double angle = tumble.angle;
			if (tumble.type == 0) Tumbletexture.scaled(0.1).rotated(angle).drawAt(tumble.pos);
			else Tumbletexture.scaled(0.1).rotated(angle).drawAt(tumble.pos);
		}

		///壁
		Rect{ 45,0,5,600 }.draw(Palette::Pink);
		Rect{ 45,0,705,5 }.draw(Palette::Pink);
		Rect{ 750,0,5,600 }.draw(Palette::Pink);

		///陣地描画

		for (int32 i = 0; i < 3; i++) {
			for (int32 j = 0; j < 7; j++) {
				if (!(j == getData().houseX && i == getData().houseY)) {
					if (getData().area[i][j] == 0) {
						Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.draw(Palette::Gray).drawFrame(3, 0);
					}
					else if (getData().area[i][j] == 1) {
						/*Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.draw(Palette::Blue).drawFrame(3, 0);*/
						Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.drawFrame(3, 0);
						if (shippohantei) {
							shippo_kuro.scaled(0.25).drawAt(100 + 100 * j, i * 100 + 55);
						}
						else {
							shippo_kuro.mirrored().scaled(0.25).drawAt(100 + 100 * j, i * 100 + 55);
						}
					}
					else if (getData().area[i][j] == 2) {
						/*Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.draw(Palette::Red).drawFrame(3, 0);*/
						Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.drawFrame(3, 0);
						if (shippohantei) {
							shippo_gold.scaled(0.25).drawAt(100 + 100 * j, i * 100 + 55);
						}
						else {
							shippo_gold.mirrored().scaled(0.25).drawAt(100 + 100 * j, i * 100 + 55);
						}
					}
				}
			}
		}

		Rect{ 50 + 100 * getData().houseX,getData().houseY * 100 + 5,100,100 }.drawFrame(3, 0);

		for (const auto& item : items)
		{
			// アイテムを描く（サイズは 0.5 倍）
			itemTextures[item.type].scaled(0.5).drawAt(item.pos);
		}

		//// プレイヤーテクスチャを指定の位置で回転して描画
		enemytexture.scaled(0.7).rotated(90_deg).drawAt(enemyPos);
		dogtexture.scaled(0.7).rotated(90_deg).drawAt(playerPos);

		// 猫を描画する
		for (const auto& cat : cats)
		{
			if (cat.type == 0) cattexture.mirrored().scaled(0.5).drawAt(cat.pos);
			else cattexture.scaled(0.5).drawAt(cat.pos);
		}

		///家
		housetexture.scaled(0.7).drawAt(getData().houseX * 100 + 100, getData().houseY * 100 + 50);

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
	Texture padtexture{ Resource(U"material/nikukyu_kuro.png") };
	Texture enemypadtexture{ Resource(U"material/nikukyu_pink.png")};
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
	
	double lastShootTime = 0.0;

	///敵機ショット
	Array<Bullet> enemyBullets;
	/*double enemyBulletSpeed = 500.0;
	double enemycooldownTime = 3.0;*/
	double enemylastShootTime = 0.0;

	///敵機ステータス更新
	double statuscooldownTime = 20.0;
	double statuslastupdateTime = 0.0;

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
	};

	// アイテムが毎秒何ピクセルの速さで落下するか
	double itemSpeed = 400.0;
	// 何秒ごとにアイテムが出現するか
	double itemSpawnTime = 2.5;
	// 前回の食べ物の出現から何秒経過したか
	double timeAccumulator = 0.0;

	///猫
	struct Cat
	{
		Vec2 pos;

		size_t type = 0;
	};
	Array<Cat> cats;

	Texture cattexture{ U"🐈"_emoji };

	// 猫が毎秒何ピクセルの速さで移動するか
	double catSpeed = 200.0;
	// 前回の猫の出現から何秒経過したか
	double cattimeAccumulator = 0.0;
	// 何秒ごとに猫が出現するか
	double catSpawnTime = 10.0;
	// 前回の食べ物の出現から何秒経過したか
	double catAccumulator = 0.0;


	/*///陣地関連
	Grid<int32> area;

	///家関連
	int32 houseX = 0;
	int32 houseY = 0;
	int32 houseCount = 0;*/
	Texture housetexture{ U"🏡"_emoji };

	bool hantei = true;

	///背景関連
	struct Tumble
	{
		Vec2 pos;
		size_t type = 0;

		double angle = 0;
	};
	Array<Tumble> tumbles;

	Texture Tumbletexture{Resource(U"material/tumble.png")};

	// タンブルが毎秒何ピクセルの速さで移動するか
	double tumbleSpeed = 100.0;
	// 前回のタンブルの出現から何秒経過したか
	double tumbletimeAccumulator = 0.0;
	// 何秒ごとにタンブルが出現するかddddddaa
	double tumbleSpawnTime = 2.0;

	Texture yuyaketexture{ Resource(U"material/yuyake.jpg") };

	Texture shippo_kuro{ Resource(U"material/shippo_kuro.png") };
	Texture shippo_gold{ Resource(U"material/shippo_gold.png") };

	// 前回のしっぽチェンジから何秒経過したか
	double shippotimeAccumulator = 0.0;
	// 何秒ごとにしっぽがチェンジするか
	double shippoTime = 0.5;

	bool shippohantei = true;

};

// 猫バトルシーン
class Neko : public App::Scene
{
public:

	// コンストラクタ（必ず実装）
	Neko(const InitData& init)
		: IScene{ init }
	{
		Window::Resize(900, 600);
	}

	~Neko()
	{
	}


	// 更新関数（オプション）
	void update() override
	{
		{
			ClearPrint();
			Print << U"自機弾: " << playerBullets.size();
			Print << U"敵機弾: " << enemyBullets.size();
			Print << U"クールダウン: " << getData().cooldownTime;
			Print << U"弾速度: " << getData().BulletSpeed;
			Print << U"HP: " << getData().hp;
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
		if (currentTime - lastShootTime >= getData().cooldownTime)
		{
			if (MouseL.down()) {

				Vec2 cursorPos = Cursor::Pos();
				Vec2 direction = (cursorPos - playerPos).normalized(); // プレイヤーからカーソルへの正規化されたベクトル

				Vec2 bulletVelocity = direction * getData().BulletSpeed;

				playerBullets << Bullet(playerPos.movedBy(0, -50), bulletVelocity);
				lastShootTime = currentTime;
			}
		}

		// 自機ショットを移動させる
		for (auto& playerBullet : playerBullets)
		{
			// 弾のあたり判定円
			const Circle BulletCircle{ playerBullet.pos, 30 };

			if (playerBullet.pos.y < 0 && playerBullet.velocity.y < 0 || 590 < playerBullet.pos.y && playerBullet.velocity.y > 0)
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

		// 画面外に出た自機ショットを削除するa
		playerBullets.remove_if([](const Bullet& bullet) { return (700 < bullet.pos.y); });

		// イテレータで全ての弾をたどる
		for (auto& playerBullet : playerBullets)
		{
			// 弾のあたり判定円
			const Circle BulletCircle{ playerBullet.pos, 30 };
			// 敵のあたり判定円
			const Circle NekoCircle{ enemyPos, 30 };
			if (BulletCircle.intersects(NekoCircle)) {
				changeScene(U"Game");
			}

		}

		// 敵機ショットの発射
		// 現在の時刻を取得
		if (currentTime - enemylastShootTime >= enemycooldownTime) {

			Vec2 enemyAim = playerPos;

			Vec2 direction = (enemyAim - enemyPos).normalized(); // 敵からランダムな位置への正規化されたベクトル
			Vec2 enemybulletVelocity = direction * enemyBulletSpeed;
			enemyBullets << Bullet(enemyPos.movedBy(0, 50), enemybulletVelocity);
			enemylastShootTime = currentTime;
		}

		// 敵機ショットを移動させる
		for (auto& enemyBullet : enemyBullets)
		{
			if (enemyBullet.pos.y < 0 && enemyBullet.velocity.y < 0 || 590 < enemyBullet.pos.y && enemyBullet.velocity.y > 0)
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

		// イテレータで全ての敵の弾をたどる
		for (auto& enemyBullet : enemyBullets)
		{
			// 弾のあたり判定円
			const Circle BulletCircle{ enemyBullet.pos, 30 };
			// プレイヤーのあたり判定円
			const Circle playerCircle{ playerPos, 30 };
			if (BulletCircle.intersects(playerCircle)) {
				getData().hp--;
				changeScene(U"Game");
			}

		}

		// 画面外に出た敵機ショットを削除する
		enemyBullets.remove_if([](const Bullet& bullet) { return (700 < bullet.pos.y); });

		///tumbleの移動
		const double tumblemove = (tumbleSpeed * Scene::DeltaTime());

		for (auto& tumble : tumbles)
		{
			if (tumble.type == 0) {
				tumble.pos.x += tumblemove;
				tumble.angle += 5.0 * Scene::DeltaTime();
			}
			else {
				tumble.pos.x -= tumblemove;
				tumble.angle -= 5.0 * Scene::DeltaTime();
			}
		}

		///画面外の猫削除
		tumbles.remove_if([](const Tumble& tumble) { return (900 < tumble.pos.x); });
		tumbles.remove_if([](const Tumble& tumble) { return (-100 > tumble.pos.x); });

	}

	// 描画関数（オプション）
	void draw() const override
	{
		yuyaketexture.draw();

		font(U"猫バトル").draw();

		///壁
		Rect{ 45,0,5,600 }.draw(Palette::Pink);
		Rect{ 45,0,705,5 }.draw(Palette::Pink);
		Rect{ 750,0,5,600 }.draw(Palette::Pink);
		Rect{ 45,595,700,5 }.draw(Palette::Pink);

		//// プレイヤーテクスチャを指定の位置で回転して描画
		enemytexture.scaled(0.7).rotated(180_deg).drawAt(enemyPos);
		dogtexture.scaled(0.7).rotated(0_deg).drawAt(playerPos);

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
	/// 基本サイズ 50 のフォントを作成
	Font font = Font(100);
	Texture dogtexture{ U"🐕"_emoji };
	Texture enemytexture{ U"🐈"_emoji };
	Texture padtexture{ U"material/nikukyu_kuro.png" };
	Texture enemypadtexture{ U"material/nikukyu_pink.png" };
	Vec2 playerPos{ 400,550 };
	Vec2 enemyPos{ 400,50 };
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
	//// 自機ショットのスピード
	//double BulletSpeed = 500.0;
	//// 弾のクールダウン時間（秒単位）を設定
	//double cooldownTime = 5.0;
	double lastShootTime = 10.0;

	///敵機ショット
	Array<Bullet> enemyBullets;
	double enemyBulletSpeed = 1000.0;
	double enemycooldownTime = 1.0;
	double enemylastShootTime = 10.0;

	RectF shape{ 50, 100, 700, 600 };

	///背景関連
	struct Tumble
	{
		Vec2 pos;
		size_t type = 0;

		double angle = 0;
	};
	Array<Tumble> tumbles;

	Texture Tumbletexture{ Resource(U"material/tumble.aapng") };
	// タンブルが毎秒何ピクセルの速さで移動するか
	double tumbleSpeed = 100.0;
	// 前回のタンブルの出現から何秒経過したか
	double tumbletimeAccumulator = 0.0;
	// 何秒ごとにタンブルが出現するか
	double tumbleSpawnTime = 2.0;

	Texture yuyaketexture{ Resource(U"material/yuyake.jpg") };
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
		if (getData().end == 4)  getData().houseCount = 0;
		getData().end = 0;
	}


	// 更新関数（オプション）
	void update() override
	{
		
	}

	// 描画関数（オプション）
	void draw() const override
	{
		Scene::SetBackground(ColorF{ 0.3, 0.4, 0.5 });

		if(getData().end == 1) font(U"制圧完了!").draw(200, 200);
		if (getData().end == 2) font(U"制圧された;;").draw(200, 200);
		if (getData().end == 3) font(U"敗北").draw(200, 200);
		if (getData().end == 4) font(U"ハッピーエンド").draw(200, 200);

	}

private:
	/// 基本サイズ 100 のフォントを作成
	Font font = Font(100);
};

void Main()
{
	// シーンマネージャーを作成
	App manager;
	manager.add<Game>(U"Game");
	manager.add<End>(U"End");
	manager.add<Neko>(U"Neko");
	manager.add<Start>(U"Start");
	// "Game" シーンから開始
	manager.init(U"Start");

	while (System::Update())
	{
		if (not manager.update())
		{
			break;
		}
	}
}
