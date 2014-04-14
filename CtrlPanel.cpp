#include "CtrlPanel.h"
#include "Target.h"
#include "Application.h"
#include <cmath>

const int fontSize = 15;

CtrlPanel::CtrlPanel(Game& g) :
    WorldObject(PassiveType),
    _game(g),
    fpsi(),
    p(),
    settingPower(false),
    settingAngle(false),
    prevMouseX(),
    mouseOldCoord(),
    fps("",Application::getFont(Sensation),fontSize),
    player("Player : 1",Application::getFont(Sensation),fontSize),
    rotation("",Application::getFont(Sensation),fontSize),
    power("Power : ",Application::getFont(Sensation),fontSize),
    fire("FIRE",Application::getFont(Sensation),fontSize),
    gaugeBg(sf::Vector2f(100,10)),
    gaugeFill(sf::Vector2f(0,10))
{
    int xpadding = 30,ypadding = 30,lpadding = 5;
    int width = constants::windowWidth/3;
    int height = fontSize;
    //line 1
    player.setPosition  (xpadding + width*0,ypadding);
    player.setColor     (sf::Color::Black);
    fps.setPosition     (xpadding + width*1,ypadding);
    fps.setColor        (sf::Color::Black);
    fire.setPosition    (xpadding + width*2,ypadding);
    fire.setColor       (sf::Color::Black);
    //line 2
    rotation.setPosition    (xpadding + width*0,ypadding + height + lpadding*1);
    rotation.setColor       (sf::Color::Black);
    power.setPosition       (xpadding + width*1,ypadding + height + lpadding*1);
    power.setColor          (sf::Color::Black);
    gaugeBg.setPosition     (power.getGlobalBounds().left+power.getGlobalBounds().width,power.getGlobalBounds().top);
    gaugeBg.setFillColor    (sf::Color(240,240,240));
    gaugeBg.setOutlineThickness(1);
    gaugeBg.setOutlineColor (sf::Color::Black);
    gaugeFill.setPosition   (gaugeBg.getPosition());
    gaugeFill.setFillColor  (sf::Color::Blue);
}
void CtrlPanel::draw(sf::RenderTarget &target)
{
    fpsi = 1/float(fpsTimer.getElapsedTime().asSeconds());
    fpsTimer.restart();
    target.draw(rotation);
    target.draw(fps);
    target.draw(player);
    target.draw(power);
    target.draw(gaugeBg);
    target.draw(gaugeFill);
    target.draw(fire);
}
void CtrlPanel::step(float dt)
{
    fps.setString("FPS : " + std::to_string(fpsi));
    auto pl = _game.getCurrPlayer();
    if(pl == nullptr)
        return;

    if(p != _game.getPlayerIndex()+1)
    {
        p = _game.getPlayerIndex()+1;
        gaugeFill.setSize(sf::Vector2f(pl->getPower()*gaugeBg.getSize().x,gaugeBg.getSize().y));
        player.setString("Player : " + std::to_string(p));
    }
    if(settingAngle || settingPower)
    {
        sf::Vector2i mouse = sf::Mouse::getPosition(Application::getWindow());
        int delta = mouse.x-prevMouseX;
        if(settingAngle)
            pl->rotateTurret( delta );
        else
            changePower( delta,pl );
        prevMouseX = mouse.x;
        if(mouse.x <= 0) //to allow continuous moving the mouse while setting the angle
        {
            sf::Mouse::setPosition(sf::Vector2i(constants::windowWidth-2,mouse.y),Application::getWindow());
            prevMouseX = constants::windowWidth-2;
        }
        else if(mouse.x >= constants::windowWidth-1)
        {
            sf::Mouse::setPosition(sf::Vector2i(1,mouse.y),Application::getWindow());
            prevMouseX = 0;
        }
        if(mouse.y <= 0)
        {
            sf::Mouse::setPosition(sf::Vector2i(mouse.x,constants::windowHeight-2),Application::getWindow());
        }
        else if(mouse.y >= constants::windowHeight-2)
        {
            sf::Mouse::setPosition(sf::Vector2i(mouse.x,1),Application::getWindow());
        }
        assert(_game.isPlayerTurn());
    }
    rotation.setString("Rotation : " + std::to_string(360-int(pl->getTurretAngle())));
}
void CtrlPanel::postSettingAngle()
{
    settingAngle = false;
    sf::Mouse::setPosition(mouseOldCoord,Application::getWindow());//set mouse back to position it was before setting angle
    Application::getWindow().setMouseCursorVisible(true);
    rotation.setStyle(sf::Text::Regular);
    auto &targtGrp = Application::getMsgStream().getGroup("TargetGroup");
    targtGrp.sendMessage(Message("Self Destruct"));
    targtGrp.clear();
}
void CtrlPanel::initSettingAngle(Player *pl)
{
    settingAngle = true;
    mouseOldCoord = sf::Mouse::getPosition(Application::getWindow());//save current coords to set it back when setting angle is over
    prevMouseX = mouseOldCoord.x;
    Application::getWindow().setMouseCursorVisible(false);
    rotation.setStyle(sf::Text::Bold);
    auto target = Application::getGame().addWorldObj(new Target(*pl));
    auto &targtGrp = Application::getMsgStream().getGroup("TargetGroup");
    targtGrp.subscribe(target);
}
void CtrlPanel::changePower(int delta,Player *pl)
{
    float powerf = pl->getPower() + 0.01*delta;
    powerf = std::max(std::min(powerf,1.0f),0.0f);
    pl->setPower(powerf);
    gaugeFill.setSize(sf::Vector2f(powerf*gaugeBg.getSize().x,gaugeBg.getSize().y));
}
void CtrlPanel::receiveMessage(const Message& msg)
{
    if(msg.ID == "WindowEvent")
    {
        const sf::Event& event = msg.getItem<sf::Event>(0);
        auto pl = _game.getCurrPlayer();
        if(pl == nullptr)
            return;
        if(_game.isPlayerTurn() && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
        {
            sf::Vector2f mcoord(event.mouseWheel.x,event.mouseButton.y);
            if( fire.getGlobalBounds().contains(sf::Vector2f(event.mouseButton.x,event.mouseButton.y)) )
                pl->fire();

            if(settingAngle)    postSettingAngle();
            else if(rotation.getGlobalBounds().contains(mcoord)) initSettingAngle(pl);

            if(settingPower)
            {
                settingPower = false;
                Application::getWindow().setMouseCursorVisible(true);
                gaugeBg.setOutlineThickness(1);
                gaugeBg.setOutlineColor(sf::Color::Black);
            }
            else if(gaugeBg.getGlobalBounds().contains(mcoord))
            {
                settingPower = true;
                Application::getWindow().setMouseCursorVisible(false);
                prevMouseX = mcoord.x;
                gaugeBg.setOutlineThickness(2);
                gaugeBg.setOutlineColor(sf::Color(200,150,150,200));
            }
        }
        else if(event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R && _game.isPlayerTurn())
        {
            if(settingAngle)    postSettingAngle();
            else                initSettingAngle(pl);
        }
        else if(event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space && _game.isPlayerTurn())
            pl->fire();
        else if(_game.isPlayerTurn() && event.type == sf::Event::MouseWheelMoved)
        {
            sf::Vector2f mcoord(event.mouseWheel.x,event.mouseButton.y);
            if(gaugeBg.getGlobalBounds().contains(mcoord))
                changePower(event.mouseWheel.delta,pl);
        }
    }
}