#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/string.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/GJLevelScoreCell.hpp>
#include <Geode/modify/CommentCell.hpp>
#include <Geode/modify/FriendsProfilePage.hpp>

using namespace geode::prelude;

static std::unordered_set<int> g_userList;
static bool g_userListLoaded = false;

class $modify(MenuLayer) {
    
    bool init() {
        if (!MenuLayer::init()) return false;
        
        if (g_userListLoaded) return true;
        
        auto accountManager = GJAccountManager::sharedState();
        int accountID = accountManager->m_accountID;
        std::string gjp2 = accountManager->m_GJP2;
        
        web::WebRequest req = web::WebRequest();
        std::string formBody = fmt::format(
            "accountID={}&gjp2={}&secret={}",
            accountID,
            gjp2,
            "Wmfd2893gb7"
        );
        req.bodyString(formBody);
        req.userAgent("");
        req.header("Content-Type", "application/x-www-form-urlencoded");
        
        std::string url = "http://www.boomlings.com/database/getGJUserList20.php";
        
        auto listener = new EventListener<web::WebTask>;
        listener->bind([] (web::WebTask::Event* e) {
            if (web::WebResponse* value = e->getValue()) {
                std::string response = value->string().unwrapOr("");

                g_userList.clear();
                
                auto users = utils::string::split(response, "|");
                
                for (const auto& user : users) {
                    auto fields = utils::string::split(user, ":");
                    
                    for (size_t i = 0; i < fields.size() - 1; i += 2) {
                        if (fields[i] == "16") {
                            auto userAccountID = utils::numFromString<int>(fields[i + 1]);
                            if (userAccountID.isOk()) {
                                g_userList.insert(userAccountID.unwrap());
                            }
                            break;
                        }
                    }
                }
                
                g_userListLoaded = true;
            }
        });
        
        listener->setFilter(req.post(url));
        
        return true;
    }
};

class $modify(GJLevelScoreCell) {
    
    void loadFromScore(GJUserScore* score) {
        GJLevelScoreCell::loadFromScore(score);
        
        if (g_userListLoaded && score) {
            int accountID = score->m_accountID;
            
            if (g_userList.find(accountID) != g_userList.end()) {
                auto sprite = CCSprite::createWithSpriteFrameName("GJ_sFriendsIcon_001.png");
                
                if (sprite) {
                    sprite->setScale(0.750f);
                    sprite->setPosition(ccp(326.625, 30.375));
                    sprite->setZOrder(10);
                    this->addChild(sprite);
                }
            }
        }
    }
};

class $modify(MyCommentCell, CommentCell) {
    
    void loadFromComment(GJComment* comment) {
        CommentCell::loadFromComment(comment);
        
        if (g_userListLoaded && comment) {
            int accountID = comment->m_accountID;
            
            if (g_userList.find(accountID) != g_userList.end()) {
                if (auto usernameMenu = this->getChildByIDRecursive("username-menu")) {
                    auto menuSprite = CCSprite::createWithSpriteFrameName("GJ_sFriendsIcon_001.png");
                    
                    if (menuSprite) {
                        menuSprite->setScale(0.6f);
                        menuSprite->setPosition(ccp(-60.0f, 0.0f));
                        
                        if (auto percentageLabel = usernameMenu->getChildByID("percentage-label")) {
                            usernameMenu->insertBefore(menuSprite, percentageLabel);
                        }

                        else if (auto modBadge = usernameMenu->getChildByID("mod-badge")) {
                            usernameMenu->insertBefore(menuSprite, modBadge);
                        }

                        else {
                            usernameMenu->addChild(menuSprite);
                        }
                    }
                }
            }
        }
    }
};

class $modify(FriendsProfilePage) {
    
    void getUserListFinished(cocos2d::CCArray* users, UserListType type) {
        FriendsProfilePage::getUserListFinished(users, type);
        
        if (type == UserListType::Friends && users) {
            g_userList.clear();
            
            for (auto userScore : CCArrayExt<GJUserScore*>(users)) {
                g_userList.insert(userScore->m_accountID);
            }
            
            g_userListLoaded = true;
        }
    }
};