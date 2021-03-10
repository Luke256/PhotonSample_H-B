#include "SceneMaster.hpp"

/// <summary>
/// 共通データ
/// シーン間で引き継ぐ用
/// </summary>
namespace Common {
    enum class Scene {
        Title,  // タイトルシーン
        Match,   // マッチングシーン
        Game //ゲーム本編
    };
    enum Turn{
        Self,
        Enemy,
        Win,
        Lose
    };


    /// <summary>
    /// ゲームデータ
    /// </summary>
    class GameData {
    private:
        // ランダムルームのフィルター用
        ExitGames::Common::Hashtable m_hashTable;
        
        
        Turn CurrentTurn=Turn::Self;

    public:
        bool isHost=true;
        GameData()
        {
            m_hashTable.put(L"gameType", L"photonSample");
        }

        ExitGames::Common::Hashtable& GetCustomProperties() {
            return m_hashTable;
        }
        
        /// <summary>
        /// 現在のターンを取得
        /// </summary>
        Turn getCurrentTurn(){
            return CurrentTurn;
        }
        
        /// <summary>
        /// ターンを変更
        /// </summary>
        Turn setCurrentTurn(Turn turn){
            CurrentTurn=turn;
            return CurrentTurn;
        }
    };
}

namespace EventCode::Game{
const nByte Initial=1;
const nByte Request=2;
const nByte Replay=3;
};

using MyScene = Utility::SceneMaster<Common::Scene, Common::GameData>;

namespace Sample {
    class Title : public MyScene::Scene {
    private:
        s3d::RectF m_startButton;
        s3d::RectF m_exitButton;

        s3d::Transition m_startButtonTransition;
        s3d::Transition m_exitButtonTransition;

    public:
        Title(const InitData& init_)
            : IScene(init_)
            , m_startButton(s3d::Arg::center(s3d::Scene::Center()), 300, 60)
            , m_exitButton(s3d::Arg::center(s3d::Scene::Center().movedBy(0, 100)), 300, 60)
            , m_startButtonTransition(s3d::SecondsF(0.4), s3d::SecondsF(0.2))
            , m_exitButtonTransition(s3d::SecondsF(0.4), s3d::SecondsF(0.2)) {}

        void update() override {
            m_startButtonTransition.update(m_startButton.mouseOver());
            m_exitButtonTransition.update(m_exitButton.mouseOver());

            if (m_startButton.mouseOver() || m_exitButton.mouseOver()) {
                s3d::Cursor::RequestStyle(s3d::CursorStyle::Hand);
            }

            if (m_startButton.leftClicked()) {
                changeScene(Common::Scene::Match);
                return;
            }

            if (m_exitButton.leftClicked()) {
                s3d::System::Exit();
                return;
            }
        }

        void draw() const override {
            const s3d::String titleText = U"ヒットアンドブロー";
            const s3d::Vec2 center(s3d::Scene::Center().x, 120);
            s3d::FontAsset(U"Title")(titleText).drawAt(center.movedBy(4, 6), s3d::ColorF(0.0, 0.5));
            s3d::FontAsset(U"Title")(titleText).drawAt(center);

            m_startButton.draw(s3d::ColorF(1.0, m_startButtonTransition.value())).drawFrame(2);
            m_exitButton.draw(s3d::ColorF(1.0, m_exitButtonTransition.value())).drawFrame(2);

            s3d::FontAsset(U"Menu")(U"接続する").drawAt(m_startButton.center(), s3d::ColorF(0.25));
            s3d::FontAsset(U"Menu")(U"おわる").drawAt(m_exitButton.center(), s3d::ColorF(0.25));
        }
    };

    class Match : public MyScene::Scene {
    private:
        s3d::RectF m_exitButton;

        s3d::Transition m_exitButtonTransition;

        void ConnectReturn(int errorCode, const ExitGames::Common::JString& errorString, const ExitGames::Common::JString& region, const ExitGames::Common::JString& cluster) override {
            if (errorCode) {
                s3d::Print(U"接続出来ませんでした");
                changeScene(Common::Scene::Title);  // タイトルシーンに戻る
                return;
            }

            s3d::Print(U"接続しました");
            GetClient().opJoinRandomRoom(getData().GetCustomProperties(), 2);   // 第2引数でルームに参加できる人数を設定します。
        }

        void DisconnectReturn() override {
            s3d::Print(U"切断しました");
            changeScene(Common::Scene::Title);  // タイトルシーンに戻る
        }

        void CreateRoomReturn(int localPlayerNr,
            const ExitGames::Common::Hashtable& roomProperties,
            const ExitGames::Common::Hashtable& playerProperties,
            int errorCode,
            const ExitGames::Common::JString& errorString) override {
            if (errorCode) {
                s3d::Print(U"部屋を作成出来ませんでした");
                Disconnect();
                return;
            }

            s3d::Print(U"部屋を作成しました！");
        }

        void JoinRandomRoomReturn(int localPlayerNr,
            const ExitGames::Common::Hashtable& roomProperties,
            const ExitGames::Common::Hashtable& playerProperties,
            int errorCode,
            const ExitGames::Common::JString& errorString) override {
            if (errorCode) {
                s3d::Print(U"部屋が見つかりませんでした");

                CreateRoom(L"", getData().GetCustomProperties(), 2);

                s3d::Print(U"部屋を作成します...");
                return;
            }

            s3d::Print(U"部屋に接続しました!");
            //相手からなのでターンはEnemy
            getData().setCurrentTurn(Common::Turn::Enemy);
            //ホストは相手
            getData().isHost=false;
            
            // この下はゲームシーンに進んだり対戦相手が設定したりする処理を書きます。
            changeScene(Common::Scene::Game);  // ゲームシーンに進む
        }

        void JoinRoomEventAction(int playerNr, const ExitGames::Common::JVector<int>& playernrs, const ExitGames::LoadBalancing::Player& player) override {
            // 部屋に入室したのが自分の場合、早期リターン
            if (GetClient().getLocalPlayer().getNumber() == player.getNumber()) {
                return;
            }

            s3d::Print(U"対戦相手が入室しました。");
            // この下はゲームシーンに進んだり設定したりする処理を書きます。
            changeScene(Common::Scene::Game);  // ゲームシーンに進む
        }

        void LeaveRoomEventAction(int playerNr, bool isInactive) override {
            s3d::Print(U"対戦相手が退室しました。");
            changeScene(Common::Scene::Title);  // タイトルシーンに戻る
        }

        /// <summary>
        /// 今回はこの関数は必要ない(何も受信しない為)
        /// </summary>
        void CustomEventAction(int playerNr, nByte eventCode, const ExitGames::Common::Object& eventContent) override {
            s3d::Print(U"受信しました");
        }

    public:
        Match(const InitData& init_)
            : IScene(init_)
            , m_exitButton(s3d::Arg::center(s3d::Scene::Center().movedBy(300, 200)), 300, 60)
            , m_exitButtonTransition(s3d::SecondsF(0.4), s3d::SecondsF(0.2)) {
            Connect();  // この関数を呼び出せば接続できる。
            s3d::Print(U"接続中...");

            GetClient().fetchServerTimestamp();
        }

        void update() override {
            m_exitButtonTransition.update(m_exitButton.mouseOver());

            if (m_exitButton.mouseOver()) {
                s3d::Cursor::RequestStyle(s3d::CursorStyle::Hand);
            }

            if (m_exitButton.leftClicked()) {
                Disconnect();   // この関数を呼び出せば切断できる。
                return;
            }
        }

        void draw() const override {
            m_exitButton.draw(s3d::ColorF(1.0, m_exitButtonTransition.value())).drawFrame(2);

            s3d::FontAsset(U"Menu")(U"タイトルに戻る").drawAt(m_exitButton.center(), s3d::ColorF(0.25));
        }
    };
    class Game : public MyScene::Scene{
    private:
        int32 Selecting;
        s3d::Array<s3d::Array<int32>>MyRequests;
        s3d::Array<s3d::Array<int32>>EnemyRequests;
        s3d::Array<s3d::int32>List;
        s3d::Array<s3d::int32>Answer;
        s3d::Array<s3d::int32>Enemy;
        s3d::Font debug;
        s3d::Font Logfont;
        
        //自分の答えを送る(相手側で処理できるようにするため)
        void Initial() {
            ExitGames::Common::Dictionary<nByte, int32> dic;
            dic.put(1, Answer[0]);
            dic.put(2, Answer[1]);
            dic.put(3, Answer[2]);
            dic.put(4, Answer[3]);

            GetClient().opRaiseEvent(true, dic, EventCode::Game::Initial);
        }
        
        //リクエスト送信
        void SendOpponent() {
            ExitGames::Common::Dictionary<nByte, int32> dic;
            dic.put(1, List[0]);
            dic.put(2, List[1]);
            dic.put(3, List[2]);
            dic.put(4, List[3]);

            GetClient().opRaiseEvent(true, dic, EventCode::Game::Request);
        }
        
        //受信処理
        void CustomEventAction(int playerNr, nByte eventCode, const ExitGames::Common::Object& eventContent) override {
            switch(eventCode){
            case EventCode::Game::Initial:{
                auto dic = ExitGames::Common::ValueObject<ExitGames::Common::Dictionary<nByte,int32>>(eventContent).getDataCopy();
                Enemy[0]=s3d::int32(*dic.getValue(1));
                Enemy[1]=s3d::int32(*dic.getValue(2));
                Enemy[2]=s3d::int32(*dic.getValue(3));
                Enemy[3]=s3d::int32(*dic.getValue(4));
                
                s3d::Print(Enemy); //受け取った答えを表示
                    break;
            }
            case EventCode::Game::Request:{
                auto dic = ExitGames::Common::ValueObject<ExitGames::Common::Dictionary<nByte,int32>>(eventContent).getDataCopy();
                s3d::Array<int32>Request;
                for(auto i: s3d::step(4)){
                    Request<<s3d::int32(*dic.getValue(i+1));
                }
                AddHistory(Request);
                
                //ターンが変わる
                if(getData().getCurrentTurn()==Common::Turn::Self || getData().getCurrentTurn()==Common::Turn::Enemy){
                    getData().setCurrentTurn(Common::Turn::Self);
                }
                break;
            }
            case EventCode::Game::Replay:{// 再戦
                getData().setCurrentTurn(Common::Turn::Enemy);
                changeScene(Common::Scene::Game);
            }

            default:
                break;
            }
        }
        
        void SelfUpdate(){
            debug(s3d::ToString(Selecting)).draw(10,10);
            
            
            for(auto i : s3d::step(6)){
                if(s3d::SimpleGUI::Button(s3d::ToString(i+1), s3d::Vec2(50,200+i*50),100) && Selecting<4){
                    List[Selecting]=i+1;
                    ++Selecting;
                }
            }
            if(s3d::SimpleGUI::Button(U"back", s3d::Vec2(50,500),100,(Selecting!=0)) && Selecting>0){
                List[Selecting]=0;
                --Selecting;
            }
            if(s3d::SimpleGUI::Button(U"send", s3d::Vec2(50,550),100,(Selecting==4))){
                SendOpponent();
                AddHistory(List);
                if(getData().getCurrentTurn()==Common::Turn::Self || getData().getCurrentTurn()==Common::Turn::Enemy){
                    getData().setCurrentTurn(Common::Turn::Enemy);
                }
                List.fill(0);
                Selecting=0;
            }
        }
        
        void AddHistory(s3d::Array<s3d::int32> List_){
            //自分のターン
            if(getData().getCurrentTurn() == Common::Turn::Self){
                s3d::Array<s3d::int32>History=List_;
                History<<0; //ヒット
                History<<0; //ブロー
                for(auto i :s3d::step(4)){
                    s3d::int32 result=0;
                    for(auto j : s3d::step(4)){
                        if(Enemy[j]==List_[i]){
                            if(i==j) result=s3d::Max(result,2);
                            else result=s3d::Max(result,1);
                        }
                    }
                    if(result==1){
                        ++History[5];
                    }
                    else if(result==2){
                        ++History[4];
                    }
                }
                MyRequests<<History;
                if(History[4]==4){ //勝利
                    getData().setCurrentTurn(Common::Turn::Win);
                }
            }
            //相手のターン
            else if(getData().getCurrentTurn() == Common::Turn::Enemy){
                s3d::Array<s3d::int32>History=List_;
                History<<0; //ヒット
                History<<0; //ブロー
                for(auto i :s3d::step(4)){
                    s3d::int32 result=0;
                    for(auto j : s3d::step(4)){
                        if(Answer[j]==List_[i]){
                            if(i==j) result=s3d::Max(result,2);
                            else result=s3d::Max(result,1);
                        }
                    }
                    if(result==1){
                        ++History[5];
                    }
                    else if(result==2){
                        ++History[4];
                    }
                }
                EnemyRequests<<History;
                if(History[4]==4){ //敗北
                    getData().setCurrentTurn(Common::Turn::Lose);
                }
            }
        }
        
        void EnemyUpdate(){
            //何もしない
        }
        
        
    public:
        Game(const InitData& init_) : IScene(init_),
        Selecting(0),
        List(4,0),
        debug(50),
        Logfont(50),
        Answer(4,0),
        Enemy(4,0)
        {
            for(auto i : s3d::step(4))Answer[i]=s3d::Random(1, 6);
            //自分の答えを送信
            Initial();
        }
        
        void update() override{
            //ボタン(飾り)
            //自分のターンの場合は上書きする
            for(auto i : s3d::step(6)){
                s3d::SimpleGUI::Button(s3d::ToString(i+1), s3d::Vec2(50,200+i*50),100,false);
            }
            s3d::SimpleGUI::Button(U"back", s3d::Vec2(50,500),100,false);
            s3d::SimpleGUI::Button(U"send", s3d::Vec2(50,550),100,false);
            
            if(getData().getCurrentTurn()==Common::Turn::Self) SelfUpdate();
            else if(getData().getCurrentTurn()==Common::Turn::Enemy) EnemyUpdate();
            else if(getData().isHost){
                //リトライ
                if(s3d::SimpleGUI::Button(U"Replay", s3d::Vec2(s3d::Scene::Width()/2-50,s3d::Scene::Height()/2+200))){
                    //相手にリトライのリクエストを送信
                    ExitGames::Common::Dictionary<nByte, int32> dic; //送る情報はないのでそのまま
                    
                    GetClient().opRaiseEvent(true, dic, EventCode::Game::Replay);
                    
                    getData().setCurrentTurn(Common::Turn::Self);
                    
                    //リフレッシュ
                    changeScene(Common::Scene::Game);
                }
            }
        }
        
        void draw()const override{
            for(auto i : s3d::step(4)){
                s3d::Rect(250,100+i*70,60).draw().drawFrame(3,0,(Selecting==i ? s3d::Palette::Orange : s3d::Palette::Silver));
                if(List[i]){
                    Logfont(s3d::ToString(List[i])).drawAt(280,130+i*70,s3d::Palette::Black);
                }
            }
            
            //自分の送信履歴・結果の表示
            for(auto i : s3d::step(MyRequests.size())){
                for(auto j : s3d::step(4))Logfont(s3d::ToString(MyRequests[i][j])).draw(350+i*55,40+j*55,s3d::Palette::Black);
                Logfont(s3d::ToString(MyRequests[i][5])).draw(350+i*55,260,s3d::Palette::Blue);
                Logfont(s3d::ToString(MyRequests[i][4])).draw(350+i*55,315,s3d::Palette::Gold);
            }
            
            //相手の送信履歴・結果の表示
            for(auto i : s3d::step(EnemyRequests.size())){
                for(auto j : s3d::step(4))Logfont(s3d::ToString(EnemyRequests[i][j])).draw(350+i*55,380+j*55,s3d::Palette::Black);
                Logfont(s3d::ToString(EnemyRequests[i][5])).draw(350+i*55,600,s3d::Palette::Blue);
                Logfont(s3d::ToString(EnemyRequests[i][4])).draw(350+i*55,655,s3d::Palette::Gold);
            }
            s3d::Line(330,375,s3d::Scene::Width()-20,370).draw(s3d::Palette::Black);
            
            //勝敗の表示
            if(getData().getCurrentTurn()==Common::Turn::Win){//勝利
                s3d::Rect(s3d::Scene::Width()/2-200,s3d::Scene::Height()/2-150,400,300).draw(s3d::ColorF(1,1,1,0.5));
                Logfont(U"WIN!").drawAt(s3d::Scene::CenterF(),s3d::Palette::Red);
            }
            if(getData().getCurrentTurn()==Common::Turn::Lose){//敗北
                s3d::Rect(s3d::Scene::Width()/2-200,s3d::Scene::Height()/2-150,400,300).draw(s3d::ColorF(1,1,1,0.5));
                Logfont(U"LOSE...").drawAt(s3d::Scene::CenterF(),s3d::Palette::Blue);
            }
            
        }
    };
}

void Main() {
    // タイトルを設定
    s3d::Window::SetTitle(U"ヒットアンドブロー");

    // ウィンドウの大きさを設定
    s3d::Window::Resize(1280, 720);

    // 背景色を設定
    s3d::Scene::SetBackground(s3d::ColorF(0.4, 0.7, 1));

    // 使用するフォントアセットを登録
    s3d::FontAsset::Register(U"Title", 120, s3d::Typeface::Heavy);
    s3d::FontAsset::Register(U"Menu", 30, s3d::Typeface::Regular);

    // シーンと遷移時の色を設定
    MyScene manager(L"d4aa9956-43ca-4060-8758-52c497ed4209", L"1.0");

    manager.add<Sample::Title>(Common::Scene::Title)
        .add<Sample::Match>(Common::Scene::Match)
        .add<Sample::Game>(Common::Scene::Game)
        .setFadeColor(s3d::ColorF(1.0));

    while (s3d::System::Update()) {
        if (!manager.update()) {
            break;
        }
    }
}
