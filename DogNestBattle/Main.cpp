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
	double cooldownTime = 3.0;

	///敵機ステータス
	double enemyBulletSpeed = 600.0;
	double enemycooldownTime = 2.5;

	///エンド判定
	int32 end = 0;

	///タイム機能
	double startTime = 0;
	double nowTime = 0;

};

using App = SceneManager<String,GameData>;

// アイテムの種類
constexpr size_t NumItems = 2;

/// エフェクト
struct RingEffect : IEffect
{
	Vec2 m_pos;

	ColorF m_color;

	explicit RingEffect(const Vec2& pos)
		: m_pos{ pos }
		, m_color{ Palette::Green} {}

	bool update(double t) override
	{
		// イージング
		const double e = EaseOutExpo(t);

		Circle{ m_pos, (e * 50) }.drawFrame((20.0 * (1.0 - e)), m_color);

		return (t < 0.3);
	}
};

// スタートシーン
class Start : public App::Scene
{
public:

	// コンストラクタ（必ず実装）
	Start(const InitData& init)
		: IScene{ init }
	{
		/*Window::Resize(900, 600);*/
	}

	~Start()
	{
		
	}


	// 更新関数（オプション）
	void update() override
	{

		if (hantei == true) {
			button.play();
			changeScene(U"Game", 0.2s);
		}

		// 経過時間の蓄積
		cattimeAccumulator += Scene::DeltaTime();

		while (catSpawnTime <= cattimeAccumulator)
		{
			int32 r = Random(2);
			if (r == 0){
				// アイテムを左に出現させる
				cats << Cat{ .pos = Vec2{ -50, Random(50,550)},.type = 0};
			}
			else if(r == 1) {
				// アイテムを右に出現させる
				cats << Cat{ .pos = Vec2{ 850, Random(50,550)},.type = 1};
			}
			else {
				// アイテムを左に出現させる
				cats << Cat{ .pos = Vec2{ -50, Random(50,550)},.type = 2 };
			}

			/*catSpeed += 50;*/
			// 経過時間を減らす
			cattimeAccumulator -= catSpawnTime;
		}

		///猫の移動
		const double catmove = (catSpeed * Scene::DeltaTime());

		for (auto& cat : cats)
		{
			if (cat.type % 2 == 0) {
				cat.pos.x += catmove;
				cat.angle += 10.0 * Scene::DeltaTime();
			}
			else {
				cat.pos.x -= catmove;
				cat.angle -= 10.0 * Scene::DeltaTime();
			}
		}

		///画面外の猫削除
		cats.remove_if([](const Cat& cat) { return (900 < cat.pos.x); });
		cats.remove_if([](const Cat& cat) { return (-100 > cat.pos.x); });


	}

	// 描画関数（オプション）
	void draw() const override
	{
		howling.play();
		kouyatexture.scaled(0.25).draw();
		if (SimpleGUI::Button(U"スタート", Vec2{ 300, 500 }, 200))///ボタン
		{
			hantei = true;
		}

		// 猫(犬)を描画する
		for (const auto& cat : cats)
		{
			if (cat.type == 0) cattexture.mirrored().scaled(0.05).rotated(cat.angle).drawAt(cat.pos);
			else if(cat.type == 1) playertexture.scaled(0.2).rotated(cat.angle).drawAt(cat.pos);
			else enemytexture.scaled(0.2).rotated(cat.angle).drawAt(cat.pos);
		}

		font(U"Dog Nest Battles").draw(200, 100,Palette::Black);
		font2(U"feat. Neko").draw(290, 200, Palette::Black);
	}

private:
	/// 基本サイズ 100 のフォントを作成
	Font font = Font (50,U"material/LightNovelPOPv2.otf" );
	Font font2 = Font (40, U"material/LightNovelPOPv2.otf");

	Texture yuyaketexture{ Resource(U"material/yuyake.jpg") };
	Texture soratexture{ Resource(U"material/sora.jpg") };
	Texture moritexture{ Resource(U"material/mori.jpg") };
	Texture morokoshitexture{ Resource(U"material/morokoshi.jpg") };
	Texture kouyatexture{ Resource(U"material/kouya.jpg") };
	///猫
	struct Cat
	{
		Vec2 pos;

		size_t type = 0;

		double angle = 0;
	};
	Array<Cat> cats;

	Texture cattexture{Resource(U"material/neko.png")};
	Texture playertexture{ Resource(U"material/player.png") };
	Texture enemytexture{ Resource(U"material/enemy.png") };

	// 猫が毎秒何ピクセルの速さで移動するか
	double catSpeed = 800.0;
	// 前回の猫の出現から何秒経過したか
	double cattimeAccumulator = 0.0;
	// 何秒ごとに猫が出現するか
	double catSpawnTime = 0.5;
	// 前回の食べ物の出現から何秒経過したか
	double catAccumulator = 0.0;

	mutable bool hantei = false;

	///オーディオ関係
	Audio button{ Resource(U"material/select01.mp3")};
	Audio howling{ Audio::Stream, Resource(U"material/howling.mp3") };
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
			getData().startTime = Scene::Time();
			getData().nowTime = Scene::Time();
		}
		getData().GameCount++;

		Window::Resize(900, 600);
	}

	~Game()
	{
	}

	void update() override
	{
		/*{
			ClearPrint();
			Print << U"自機弾: " << playerBullets.size();
			Print << U"敵機弾: " << enemyBullets.size();
			Print << U"アイテムの個数: " << items.size();
			Print << U"アイテムのクールダウン: " << timeAccumulator;
			Print << U"クールダウン: " << getData().cooldownTime;
			Print << U"弾速度: " << getData().BulletSpeed;
			Print << U"家にぶつかった回数: " << getData().houseCount;
			Print << U"HP: " << getData().hp;
		}*/
		const double deltaTime = Scene::DeltaTime();

		// 右矢印キーが押されているかをチェック
		if (rightInput.pressed())
		{
			// 経過時間に応じて移動
			playerPos.x += moveSpeed * deltaTime;
			playerangle += 10.0 * Scene::DeltaTime();
		}
		///左も
		if (leftInput.pressed())
		{
			// 経過時間に応じて移動
			playerPos.x -= moveSpeed * deltaTime;
			playerangle -= 10.0 * Scene::DeltaTime();
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
		if (enemySpeed < 0) {
			enemyangle -= 5.0 * Scene::DeltaTime();
		}
		else {
			enemyangle += 5.0 * Scene::DeltaTime();
		}

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
				playerdog.play();
				effect.add<RingEffect>(Cursor::Pos());///エフェクト
			}
		}

		// 自機ショットを移動させる
		for (auto& playerBullet : playerBullets)
		{
			// 弾のあたり判定円
			const Circle BulletCircle{ playerBullet.pos, 25 };

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
			const Circle BulletCircle{ playerBullet.pos, 25 };


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
			enemydog.play();
			enemylastShootTime = currentTime;
		}

		// 敵機ショットを移動させる
		for (auto& enemyBullet : enemyBullets)
		{
			// 弾のあたり判定円
			const Circle BulletCircle{ enemyBullet.pos, 25 };

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
			const Circle BulletCircle{ enemyBullet.pos, 25 };

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

		//// 自機ショットと敵機ショットの衝突(テスト)
		//for (auto itPlayer = playerBullets.begin(); itPlayer != playerBullets.end(); )
		//{
		//	bool removePlayerBullet = false;

		//	const Circle playerBulletCircle{ itPlayer->pos, 25 };

		//	for (auto itEnemy = enemyBullets.begin(); itEnemy != enemyBullets.end(); )
		//	{
		//		const Circle enemyBulletCircle{ itEnemy->pos, 25 };

		//		// 円同士の衝突を検出
		//		if (playerBulletCircle.intersects(enemyBulletCircle))
		//		{
		//			removePlayerBullet = true;
		//			itEnemy = enemyBullets.erase(itEnemy);
		//		}
		//		else
		//		{
		//			++itEnemy;
		//		}
		//	}

		//	if (removePlayerBullet)
		//	{
		//		itPlayer = playerBullets.erase(itPlayer);
		//	}
		//	else
		//	{
		//		++itPlayer;
		//	}
		//}

		///敵機ステータス更新
		if (currentTime - statuslastupdateTime >= statuscooldownTime) {

			getData().enemyBulletSpeed += 150.0;
			getData().enemycooldownTime -= 0.3;
			if (getData().enemycooldownTime < 0.5) getData().enemycooldownTime = 0.5;
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

		///アイテムの当たり判定(ステータス）
		Circle playerCircle{ playerPos, 25 };
		for (auto it = items.begin(); it != items.end(); ) {
			Circle itemCircle{ it->pos, 30 };
			if (playerCircle.intersects(itemCircle)) {
				if (it->type == 0) {
					getData().BulletSpeed += 100.0;
					powerup1.play();
				}
				else {
					getData().cooldownTime -= 0.5;
					if (getData().cooldownTime < 0.5) getData().cooldownTime = 0.5;
					powerup3.play();
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
				cat1.play();
			}
			else {
				// アイテムを右に出現させる
				cats << Cat{ .pos = Vec2{ 850, 550 },.type = 1 };
				cat2.play();
			}

			catSpeed += 50;
			// 経過時間を減らす
			cattimeAccumulator -= catSpawnTime;
		}

		///猫の移動
		const double catmove = (catSpeed * Scene::DeltaTime());

		for (auto& cat : cats)
		{
			if (cat.type == 0) {
				cat.pos.x += catmove;
				cat.angle += 10.0 * Scene::DeltaTime();
			}
			else {
				cat.pos.x -= catmove;
				cat.angle -= 10.0 * Scene::DeltaTime();
			}
		}

		/////弾と猫の当たり判定
		for (auto it = cats.begin(); it != cats.end(); ) {
			Circle catCircle{ it->pos, 25 };
			bool intersectionFound = false;

			for (auto& playerBullet : playerBullets) {
				const Circle BulletCircle{ playerBullet.pos, 25 };
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
			Circle catCircle{ it->pos, 25 };
			bool intersectionFound = false;
			/*Circle playerCircle{ playerPos, 30 };*/

			if (playerCircle.intersects(catCircle)) {
				// 削除する要素の位置を記録
				it = cats.erase(it);
				intersectionFound = true;
				getData().hp--;
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

		///家にヒットした回数が500以上なら
		if(getData().houseCount >= 500) {
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

		///タイム機能
		getData().nowTime = Scene::Time() - getData().startTime;
	}

	void draw() const override
	{
		///背景
		/*Scene::SetBackground(Palette::Green);*/
		/*yuyaketexture.draw();*/
		kouyatexture.scaled(0.25).draw();
		///タンブル描画する
		for (const auto& tumble : tumbles)
		{
			double angle = tumble.angle;
			if (tumble.type == 0) Tumbletexture.scaled(0.1).rotated(angle).drawAt(tumble.pos);
			else Tumbletexture.scaled(0.1).rotated(angle).drawAt(tumble.pos);
		}

		///壁
		Rect{ 45,0,5,600 }.draw(Palette::Black);
		Rect{ 45,0,705,5 }.draw(Palette::Black);
		Rect{ 750,0,5,600 }.draw(Palette::Black);

		///陣地描画

		for (int32 i = 0; i < 3; i++) {
			for (int32 j = 0; j < 7; j++) {
				if (!(j == getData().houseX && i == getData().houseY)) {
					if (getData().area[i][j] == 0) {
						Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.draw(Palette::Gray).drawFrame(3, 0);
					}
					else if (getData().area[i][j] == 1) {
						Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.draw(Palette::Skyblue).drawFrame(3, 0);
						/*Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.drawFrame(3, 0);*/
						if (shippohantei) {
							shippo_kuro.scaled(0.25).drawAt(100 + 100 * j, i * 100 + 55);
						}
						else {
							shippo_kuro.mirrored().scaled(0.25).drawAt(100 + 100 * j, i * 100 + 55);
						}
					}
					else if (getData().area[i][j] == 2) {
						Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.draw(Palette::Orangered).drawFrame(3, 0);
						/*Rect{ 50 + 100 * j,i * 100 + 5,100,100 }.drawFrame(3, 0);*/
						if (shippohantei) {
							shippo_gold.mirrored().scaled(0.25).drawAt(100 + 100 * j, i * 100 + 55);
						}
						else {
							shippo_gold.scaled(0.25).drawAt(100 + 100 * j, i * 100 + 55);
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
		enemytexture.scaled(0.2).rotated(enemyangle).drawAt(enemyPos);
		dogtexture.scaled(0.2).rotated(playerangle).drawAt(playerPos);

		// 猫を描画する
		for (const auto& cat : cats)
		{
			if (cat.type == 0) cattexture.mirrored().scaled(0.05).rotated(cat.angle).drawAt(cat.pos);
			else cattexture.scaled(0.05).rotated(cat.angle).drawAt(cat.pos);
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

		Shape2D::Heart(60, Vec2{ 830, 100 }).draw(Palette::Red);

		font(getData().hp).drawAt(830,100);

		///タイム機能
		font2(U"Time:{:.1f}s"_fmt(getData().nowTime)).draw(760, 200, Palette::Black);

		///エフェクト
		effect.update();

		//// あたり判定円のデバッグ表示
		//{
		//	// プレイヤーのあたり判定円
		//	Circle{ playerPos, 50 }.draw(ColorF{ 1.0, 1.0, 1.0, 0.5 });

		//	for (const auto& playerBullet : playerBullets)
		//	{
		//		// 食べ物のあたり判定円
		//		Circle{ playerBullet.pos, 25 }.draw(ColorF{ 1.0, 1.0, 1.0, 0.5 });
		//	}
		//}
	}

private:
	Font font = Font(50, U"material/LightNovelPOPv2.otf");
	Font font2 = Font(27, U"material/LightNovelPOPv2.otf");
	Texture dogtexture{ Resource(U"material/player.png") };
	Texture enemytexture{ Resource(U"material/enemy.png") };
	Texture padtexture{ Resource(U"material/nikukyu_kuro.png") };
	Texture enemypadtexture{ Resource(U"material/nikukyu_pink.png")};
	Vec2 playerPos{ 400,550 };
	Vec2 enemyPos{ 400,550 };
	//移動する速度を設定
	double moveSpeed = 300.0;
	double enemySpeed = 300.0;
	double playerangle = 0;
	double enemyangle = 0;

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
	
	double lastShootTime = -2.0;

	///敵機ショット
	Array<Bullet> enemyBullets;
	/*double enemyBulletSpeed = 500.0;
	double enemycooldownTime = 3.0;*/
	double enemylastShootTime = -2.0;

	///敵機ステータス更新
	double statuscooldownTime = 5.0;
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
	double itemSpawnTime = 3.0;
	// 前回の食べ物の出現から何秒経過したか
	double timeAccumulator = 0.0;

	///猫
	struct Cat
	{
		Vec2 pos;

		size_t type = 0;

		double angle = 0;
	};
	Array<Cat> cats;

	Texture cattexture{ Resource(U"material/neko.png") };

	// 猫が毎秒何ピクセルの速さで移動するか(ステータス)
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
	// 何秒ごとにタンブルが出現するか
	double tumbleSpawnTime = 2.0;

	Texture yuyaketexture{ Resource(U"material/yuyake.jpg") };
	Texture morokoshitexture{ Resource(U"material/morokoshi.jpg") };
	Texture soratexture{ Resource(U"material/sora.jpg") };
	Texture moritexture{ Resource(U"material/mori.jpg") };
	Texture kouyatexture{ Resource(U"material/kouya.jpg") };

	Texture shippo_kuro{ Resource(U"material/shippo_kuro.png") };
	Texture shippo_gold{ Resource(U"material/shippo_gold.png") };

	// 前回のしっぽチェンジから何秒経過したか
	double shippotimeAccumulator = 0.0;
	// 何秒ごとにしっぽがチェンジするか
	double shippoTime = 0.5;

	bool shippohantei = true;

	Audio enemydog{Resource(U"material/dog1b.mp3")};
	Audio playerdog{ Resource(U"material/howling_player.mp3") };
	Audio cat1{ Resource(U"material/cat1a.mp3") };
	Audio cat2{ Resource(U"material/cat1b.mp3") };
	Audio powerup1{ Resource(U"material/powerup01.mp3") };
	Audio powerup3{ Resource(U"material/powerup03.mp3") };

	///エフェクト
	Effect effect;
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
		/*{
			ClearPrint();
			Print << U"自機弾: " << playerBullets.size();
			Print << U"敵機弾: " << enemyBullets.size();
			Print << U"クールダウン: " << getData().cooldownTime;
			Print << U"弾速度: " << getData().BulletSpeed;
			Print << U"HP: " << getData().hp;
		}*/
		const double deltaTime = Scene::DeltaTime();

		// 右矢印キーが押されているかをチェック
		if (rightInput.pressed())
		{
			// 経過時間に応じて移動
			playerPos.x += moveSpeed * deltaTime;
			playerangle += 10.0 * Scene::DeltaTime();
		}

		if (leftInput.pressed())
		{
			// 経過時間に応じて移動
			playerPos.x -= moveSpeed * deltaTime;
			playerangle -= 10.0 * Scene::DeltaTime();
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
		if (enemySpeed < 0) {
			enemyangle -= 10.0 * Scene::DeltaTime();
		}
		else {
			enemyangle += 10.0 * Scene::DeltaTime();
		}

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
				dog.play();
				effect.add<RingEffect>(Cursor::Pos());
			}
		}

		// 自機ショットを移動させる
		for (auto& playerBullet : playerBullets)
		{
			// 弾のあたり判定円
			const Circle BulletCircle{ playerBullet.pos, 25 };

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
			const Circle BulletCircle{ playerBullet.pos, 25};
			// 敵のあたり判定円
			const Circle NekoCircle{ enemyPos, 30};
			if (BulletCircle.intersects(NekoCircle)) {
				dog.play();
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
			cat.play();
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
			const Circle BulletCircle{ enemyBullet.pos, 25};
			// プレイヤーのあたり判定円
			const Circle playerCircle{ playerPos, 50};
			if (BulletCircle.intersects(playerCircle)) {
				getData().hp--;
				cat.play();
				changeScene(U"Game");
			}

		}

		// 自機ショットと敵機ショットの衝突(テスト)
		for (auto itPlayer = playerBullets.begin(); itPlayer != playerBullets.end(); )
		{
			bool removePlayerBullet = false;

			const Circle playerBulletCircle{ itPlayer->pos, 25 };

			for (auto itEnemy = enemyBullets.begin(); itEnemy != enemyBullets.end(); )
			{
				const Circle enemyBulletCircle{ itEnemy->pos, 25 };

				// 円同士の衝突を検出
				if (playerBulletCircle.intersects(enemyBulletCircle))
				{
					removePlayerBullet = true;
					itEnemy = enemyBullets.erase(itEnemy);
				}
				else
				{
					++itEnemy;
				}
			}

			if (removePlayerBullet)
			{
				itPlayer = playerBullets.erase(itPlayer);
			}
			else
			{
				++itPlayer;
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

		///アニメーション時間経過判定
		alphatimeAccumulator += Scene::DeltaTime();

		while (alphaTime <= alphatimeAccumulator)
		{
			alpha -= 0.1;

			// 経過時間を減らす
			alphatimeAccumulator -= alphaTime;
		}

		///タイム機能
		getData().nowTime = Scene::Time() - getData().startTime;
	}

	// 描画関数（オプション）
	void draw() const override
	{
		kouyatexture.scaled(0.25).draw();

		///壁
		Rect{ 45,0,5,600 }.draw(Palette::Black);
		Rect{ 45,0,705,5 }.draw(Palette::Black);
		Rect{ 750,0,5,600 }.draw(Palette::Black);
		Rect{ 45,595,700,5 }.draw(Palette::Black);

		//// プレイヤーテクスチャを指定の位置で回転して描画
		enemytexture.scaled(0.05).rotated(enemyangle).drawAt(enemyPos);
		dogtexture.scaled(0.2).rotated(playerangle).drawAt(playerPos);

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
		
		/*Circle{ 100,200,100 }.draw(HSV{ 90,alpha });*/
		Shape2D::NStar(10, 136, 102, Vec2{ 400, 220 }).draw(HSV{ 90,alpha });
		font(U"猫バトル").draw(300, 185, HSV{ 20,alpha });

		///HP
		Shape2D::Heart(60, Vec2{ 830, 100 }).draw(Palette::Red);

		font(getData().hp).drawAt(830, 100);

		///エフェクト
		effect.update();

		///タイム機能
		font2(U"Time:{:.1f}s"_fmt(getData().nowTime)).draw(760, 200, Palette::Black);
	}

private:
	/// 基本サイズ 50 のフォントを作成
	Font font = Font(50, U"material/LightNovelPOPv2.otf");
	Font font2 = Font(27, U"material/LightNovelPOPv2.otf");
	Texture dogtexture{Resource(U"material/player.png")};
	Texture enemytexture{ Resource(U"material/neko.png") };
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
	double playerangle = 0;
	double enemyangle = 0;

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
	double lastShootTime = -2.0;

	///敵機ショット(ステータス)
	Array<Bullet> enemyBullets;
	double enemyBulletSpeed = 700.0;
	double enemycooldownTime = 1.0;
	double enemylastShootTime = -1.0;

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
	Texture moritexture{ Resource(U"material/mori.jpg") };
	Texture kouyatexture{ Resource(U"material/kouya.jpg") };
	///アニメーション
	double alpha = 1.0;
	double alphatimeAccumulator = 0.0;
	double alphaTime = 0.1;

	Audio cat{Resource(U"material/cat_like2a.mp3") };
	Audio dog{ Resource(U"material/howling_player.mp3") };

	///エフェクト
	Effect effect;
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
		if (getData().end == 4) {
			getData().houseCount = 0;
		}
		getData().end = 0;

		for (int32 i = 0; i < 3; i++) {
			for (int32 j = 0; j < 7; j++) {
				getData().area[i][j] = 0;
			}
		}

		getData().hp = 3;

		// 自機ショットのスピード
		getData().BulletSpeed = 500.0;
		// 弾のクールダウン時間（秒単位）を設定
		getData().cooldownTime = 3.0;

		///敵機ステータス
		getData().enemyBulletSpeed = 600.0;
		getData().enemycooldownTime = 2.5;
		getData().GameCount = 0;
	}


	// 更新関数（オプション）
	void update() override
	{
		if (hantei == 1) changeScene(U"Start", 0.2s);
		if (hantei == 2) System::Exit();

		// 経過時間の蓄積
		///catだけど犬
		cattimeAccumulator += Scene::DeltaTime();

		while (catSpawnTime <= cattimeAccumulator)
		{
			if (RandomBool()) {
				// アイテムを左に出現させる
				cats << Cat{ .pos = Vec2{ -50, Random(50,550)},.type = 0 };
			}
			else {
				// アイテムを右に出現させる
				cats << Cat{ .pos = Vec2{ 850, Random(50,550)},.type = 1 };
			}

			/*catSpeed += 50;*/
			// 経過時間を減らす
			cattimeAccumulator -= catSpawnTime;
		}

		///猫の移動
		const double catmove = (catSpeed * Scene::DeltaTime());

		for (auto& cat : cats)
		{
			if (cat.type == 0) {
				cat.pos.x += catmove;
				cat.angle += 10.0 * Scene::DeltaTime();
			}
			else {
				cat.pos.x -= catmove;
				cat.angle -= 10.0 * Scene::DeltaTime();
			}
		}

		///画面外の猫削除
		cats.remove_if([](const Cat& cat) { return (900 < cat.pos.x); });
		cats.remove_if([](const Cat& cat) { return (-100 > cat.pos.x); });
	}

	// 描画関数（オプション）
	void draw() const override
	{
		kouyatexture.scaled(0.25).draw();

		kouyatexture.scaled(0.25).draw();
		if (SimpleGUI::Button(U"スタート画面に戻る", Vec2{ 100, 500 }, 200))///ボタン
		{
			hantei = 1;
		}

		if (SimpleGUI::Button(U"ゲームをやめる", Vec2{ 500, 500 }, 200))///ボタン
		{
			hantei = 2;
		}

		if (getData().end == 1) {
			good.play();
			font(U"巣食った!").draw(295, 200, Palette::Black);
			// 犬を描画する
			for (const auto& cat : cats)
			{
				if (cat.type == 0) dogtexture.mirrored().scaled(0.3).rotated(cat.angle).drawAt(cat.pos);
				else dogtexture.scaled(0.3).rotated(cat.angle).drawAt(cat.pos);
				///タイム機能
				font(U"ClearTime:{:.1f}s"_fmt(getData().nowTime)).draw(210, 300, Palette::Mediumvioletred);
			}
		}
		if (getData().end == 2) {
			dogbad.play();
			font(U"巣食われた").draw(295, 200, Palette::Black);
			// 敵犬を描画する
			for (const auto& cat : cats)
			{
				if (cat.type == 0) enemytexture.mirrored().scaled(0.3).rotated(cat.angle).drawAt(cat.pos);
				else enemytexture.scaled(0.3).rotated(cat.angle).drawAt(cat.pos);
			}
		}
		if (getData().end == 3)
		{
			catbad.play();
			font(U"猫が巣食った").draw(295, 200, Palette::Black);
			// 猫を描画する
			for (const auto& cat : cats)
			{
				if (cat.type == 0) cattexture.mirrored().scaled(0.1).rotated(cat.angle).drawAt(cat.pos);
				else cattexture.scaled(0.1).rotated(cat.angle).drawAt(cat.pos);
			}
		}
		if (getData().end == 4) {
			good.play();
			font(U"救われた!").draw(295, 100, Palette::Black);
			font2(U"飼い犬エンド").draw(295, 200, Palette::Black);

			housetexture.drawAt(400, 400);
			dogtexture.scaled(0.4).drawAt(200, 400);
			enemytexture.scaled(0.4).drawAt(600, 400);
		}

	}

private:
	/// 基本サイズ 50 のフォントを作成
	Font font = Font(50, U"material/LightNovelPOPv2.otf");
	Font font2 = Font(40, U"material/LightNovelPOPv2.otf");
	Texture kouyatexture{ Resource(U"material/kouya.jpg") };

	///猫
	struct Cat
	{
		Vec2 pos;

		size_t type = 0;

		double angle = 0;
	};
	Array<Cat> cats;

	Texture dogtexture{Resource(U"material/player.png")};
	Texture enemytexture{ Resource(U"material/enemy.png") };
	Texture cattexture{ Resource(U"material/neko.png") };
	Texture housetexture{ U"🏘"_emoji };

	// 猫が毎秒何ピクセルの速さで移動するか
	double catSpeed = 500.0;
	// 前回の猫の出現から何秒経過したか
	double cattimeAccumulator = 0.0;
	// 何秒ごとに猫が出現するか
	double catSpawnTime = 0.5;
	// 前回の猫の出現から何秒経過したか
	double catAccumulator = 0.0;

	mutable int32 hantei = 0;

	///オーディオ関連
	Audio catbad{ Resource(U"material/cats_fighting.mp3") };
	Audio dogbad{ Resource(U"material/barking_dog_in_dream.mp3") };
	Audio good{ Resource(U"material/party_poppers3.mp3") };

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
