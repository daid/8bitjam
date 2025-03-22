#include <sp2/engine.h>
#include <sp2/window.h>
#include <sp2/logging.h>
#include <sp2/io/resourceProvider.h>
#include <sp2/io/zipResourceProvider.h>
#include <sp2/io/fileSelectionDialog.h>
#include <sp2/audio/sound.h>
#include <sp2/audio/music.h>
#include <sp2/audio/musicPlayer.h>
#include <sp2/graphics/gui/scene.h>
#include <sp2/graphics/gui/theme.h>
#include <sp2/graphics/gui/loader.h>
#include <sp2/graphics/gui/widget/button.h>
#include <sp2/graphics/gui/widget/keynavigator.h>
#include <sp2/graphics/scene/graphicslayer.h>
#include <sp2/graphics/scene/basicnoderenderpass.h>
#include <sp2/graphics/scene/collisionrenderpass.h>
#include <sp2/graphics/textureManager.h>
#include <sp2/graphics/renderTexture.h>
#include <sp2/graphics/meshdata.h>
#include <sp2/scene/scene.h>
#include <sp2/scene/node.h>
#include <sp2/scene/camera.h>
#include <sp2/io/keybinding.h>
#include <array>

#include "main.h"
#include "editor.h"
#include "mainScene.h"
#include "ingameMenu.h"
#include "tileinfo.h"
#include "unitinfo.h"
#include "textmenu.h"


sp::P<sp::Window> window;
sp::P<sp::Node> screen_node;
sp::io::Keybinding escape_key("ESCAPE", {"Escape", "AC Back"});
Controller controller;


static void openOptionsMenu();
static void openCreditsMenu();
static void openControlsMenu();
void openMainMenu()
{
    sp::P<sp::gui::Widget> menu = sp::gui::Loader::load("gui/main_menu.gui", "MAIN_MENU");
    menu->getWidgetWithID("START")->setEventCallback([=](sp::Variant v) mutable {
        menu.destroy();
        new Scene();
    });
    menu->getWidgetWithID("EDITOR")->setEventCallback([=](sp::Variant v) mutable {
        menu.destroy();
        openEditor();
    });
    menu->getWidgetWithID("OPTIONS")->setEventCallback([=](sp::Variant v) mutable {
        menu.destroy();
        openOptionsMenu();
    });
    menu->getWidgetWithID("CREDITS")->setEventCallback([=](sp::Variant v) mutable {
        menu.destroy();
        openCreditsMenu();
    });
    menu->getWidgetWithID("CRT")->setEventCallback([=](sp::Variant v) mutable {
        if (v.getInteger()) {
            screen_node->render_data.shader = sp::Shader::get("crt.shader");
        } else {
            screen_node->render_data.shader = sp::Shader::get("internal:basic.shader");
        }
    });
    menu->getWidgetWithID("QUIT")->setEventCallback([](sp::Variant v){
        sp::Engine::getInstance()->shutdown();
    });
#ifdef EMSCRIPTEN
    menu->getWidgetWithID("QUIT")->hide();
#endif
}

static void openOptionsMenu()
{
    sp::P<sp::gui::Widget> menu = sp::gui::Loader::load("gui/main_menu.gui", "OPTIONS_MENU");
    menu->getWidgetWithID("EFFECT_VOLUME")->getWidgetWithID("VALUE")->setAttribute("caption", sp::string(int(sp::audio::Sound::getVolume())) + "%");
    menu->getWidgetWithID("EFFECT_VOLUME")->getWidgetWithID("SLIDER")->setAttribute("value", sp::string(sp::audio::Sound::getVolume()));
    menu->getWidgetWithID("EFFECT_VOLUME")->getWidgetWithID("SLIDER")->setEventCallback([=](sp::Variant v) mutable {
        menu->getWidgetWithID("EFFECT_VOLUME")->getWidgetWithID("VALUE")->setAttribute("caption", sp::string(v.getInteger()) + "%");
        sp::audio::Sound::setVolume(v.getInteger());
    });
    menu->getWidgetWithID("MUSIC_VOLUME")->getWidgetWithID("VALUE")->setAttribute("caption", sp::string(int(sp::audio::Music::getVolume())) + "%");
    menu->getWidgetWithID("MUSIC_VOLUME")->getWidgetWithID("SLIDER")->setAttribute("value", sp::string(sp::audio::Music::getVolume()));
    menu->getWidgetWithID("MUSIC_VOLUME")->getWidgetWithID("SLIDER")->setEventCallback([=](sp::Variant v) mutable {
        menu->getWidgetWithID("MUSIC_VOLUME")->getWidgetWithID("VALUE")->setAttribute("caption", sp::string(v.getInteger()) + "%");
        sp::audio::Music::setVolume(v.getInteger());
    });
    menu->getWidgetWithID("CONTROLS")->setEventCallback([=](sp::Variant v) mutable {
        menu.destroy();
        openControlsMenu();
    });
    menu->getWidgetWithID("BACK")->setEventCallback([=](sp::Variant v) mutable {
        menu.destroy();
        openMainMenu();
    });
}

class Rebinder : public sp::Node
{
public:
    Rebinder(sp::P<sp::gui::Widget> parent, sp::P<sp::io::Keybinding> binding, sp::io::Keybinding::Type bind_type)
    : sp::Node(parent), binding(binding), bind_type(bind_type)
    {
        binding->startUserRebind(bind_type);
        sp::P<sp::gui::Widget> w = getParent();
        parent->setAttribute("caption", "?");
        findNavigator(getScene()->getRoot())->disable();
    }

    virtual void onUpdate(float delta) override
    {
        if (!binding->isUserRebinding())
        {
            //TODO: Remove oldest bind of there are too many keys of this type.
            int count = 0;
            for(int n=0; binding->getKeyType(n) != sp::io::Keybinding::Type::None; n++)
                if (binding->getKeyType(n) & bind_type)
                    count += 1;
            if (count > 2)
            {
                for(int n=0; binding->getKeyType(n) != sp::io::Keybinding::Type::None; n++)
                {
                    if (binding->getKeyType(n) & bind_type)
                    {
                        binding->removeKey(n);
                        break;
                    }
            }
            }
            sp::P<sp::gui::Widget> w = getParent();
            for(int n=0; binding->getKeyType(n) != sp::io::Keybinding::Type::None; n++)
                w->setAttribute("caption", binding->getHumanReadableKeyName(n));
            findNavigator(getScene()->getRoot())->enable();
            delete this;
        }
    }

private:
    sp::P<sp::gui::KeyNavigator> findNavigator(sp::P<sp::Node> node)
    {
        if (sp::P<sp::gui::KeyNavigator>(node))
            return node;
        for(auto child : node->getChildren())
        {
            auto result = findNavigator(child);
            if (result)
                return result;
        }
        return nullptr;
    }

    sp::P<sp::io::Keybinding> binding;
    sp::io::Keybinding::Type bind_type;
};

static void openControlsMenu()
{
    sp::gui::Loader loader("gui/main_menu.gui");
    sp::P<sp::gui::Widget> menu = loader.create("CONTROLS_MENU");
    //new Rebinder(menu);

    std::array<sp::io::Keybinding::Type, 4> key_types{
        sp::io::Keybinding::Type::Keyboard,
        sp::io::Keybinding::Type::Keyboard,
        sp::io::Keybinding::Type::Controller,
        sp::io::Keybinding::Type::Controller,
    };
    for(auto keybinding : controller.all)
    {
        sp::P<sp::gui::Widget> keybinding_menu = loader.create("@CONTROLS_KEYBINDING", menu->getWidgetWithID("KEYS"));
        keybinding_menu->getWidgetWithID("NAME")->setAttribute("caption", keybinding->getLabel() + ":");
        int done = 0;
        for(int n=0; n<3; n++)
        {
            auto button = loader.create("@CONTROLS_KEYBINDING_BUTTON", keybinding_menu);
            button->setAttribute("caption", "");
            for(int m=0; keybinding->getKeyType(m) != sp::io::Keybinding::Type::None; m++)
            {
                if (!(done & (1 << m)) && (keybinding->getKeyType(m) & key_types[n]))
                {
                    done |= 1 << m;
                    button->setAttribute("caption", keybinding->getHumanReadableKeyName(m));
                    break;
                }
            }
            button->setEventCallback([=](sp::Variant v) mutable
            {
                new Rebinder(button, keybinding, key_types[n]);
            });
        }
    }
    menu->getWidgetWithID("BACK")->setEventCallback([=](sp::Variant v) mutable {
        menu.destroy();
        openOptionsMenu();
    });
}

class AutoCreditScroller : public sp::Node
{
public:
    using sp::Node::Node;

    virtual void onUpdate(float delta) override
    {
        sp::P<sp::gui::Widget> w = getParent();
        w->layout.position.y -= delta * 30.0f;
        if (w->layout.position.y < -w->layout.size.y)
            w->layout.position.y = 480.0f;
    }
};

static void openCreditsMenu()
{
    sp::P<sp::gui::Widget> menu = sp::gui::Loader::load("gui/main_menu.gui", "CREDITS_MENU");
    new AutoCreditScroller(menu->getWidgetWithID("CREDITS"));
    menu->getWidgetWithID("BACK")->setEventCallback([=](sp::Variant v) mutable {
        menu.destroy();
        openMainMenu();
    });
}

class UpdateScreenSizePass : public sp::RenderPass
{
public:
    void render(sp::RenderQueue& queue) override
    {
        queue.add([]() {
            auto size = sp::Vector2f(window->getSize());
            if (size.x / size.y > 256.0f / 224.0f) {
                size.x = size.y / 224.0f * 256.0f;
            } else {
                size.y = size.x / 256.0f * 224.0f;
            }
            auto s = sp::Shader::get("crt.shader");
            s->bind();
            s->setUniform("output_size", size);
        });
    }
};

class BasicNodeRenderPass : public sp::BasicNodeRenderPass
{
public:
    bool onPointerMove(sp::Vector2d position, int id) override { return sp::BasicNodeRenderPass::onPointerMove(fixPos(position), id); }
    bool onPointerDown(sp::io::Pointer::Button button, sp::Vector2d position, int id) override { return sp::BasicNodeRenderPass::onPointerDown(button, fixPos(position), id); }
    void onPointerDrag(sp::Vector2d position, int id) override { sp::BasicNodeRenderPass::onPointerDrag(fixPos(position), id); }
    void onPointerUp(sp::Vector2d position, int id) override { sp::BasicNodeRenderPass::onPointerUp(fixPos(position), id); }

private:
    sp::Vector2d fixPos(sp::Vector2d p) {
        auto size = window->getSize();
        double psx = double(size.y) / 224.0 * 256.0;
        p.x = p.x * double(size.x) / psx;
        return p;
    }
};

int main(int argc, char** argv)
{
    sp::P<sp::Engine> engine = new sp::Engine();
    SP_REGISTER_WIDGET("BorderPanel", BorderPanel);

    //Create resource providers, so we can load things.
    sp::io::ResourceProvider::createDefault();

    //Disable or enable smooth filtering by default, enabling it gives nice smooth looks, but disabling it gives a more pixel art look.
    sp::texture_manager.setDefaultSmoothFiltering(false);

    //Create a window to render on, and our engine.
    window = new sp::Window(256.0f/224.0f);
    window->setClearColor({0, 0, 0});
#if !defined(DEBUG) && !defined(EMSCRIPTEN)
    window->setFullScreen(true);
#endif

    sp::gui::Theme::loadTheme("default", "gui/theme/basic.theme.txt");
    new sp::gui::Scene(sp::Vector2d(256, 224));

    sp::RenderTexture screen_texture("SCREEN", {256, 224}, false);
    sp::P<sp::SceneGraphicsLayer> scene_layer = new sp::SceneGraphicsLayer(1);
    scene_layer->addRenderPass(new BasicNodeRenderPass());
#ifdef DEBUG
    scene_layer->addRenderPass(new sp::CollisionRenderPass());
#endif
    scene_layer->setTarget(&screen_texture);
    window->addLayer(scene_layer);

    auto ss = new sp::Scene("SCREEN_SCENE");
    auto sc = new sp::Camera(ss->getRoot());
    sc->setOrtographic({256.0f * 0.5f, 224.0f * 0.5f});
    screen_node = new sp::Node(ss->getRoot());
    screen_node->render_data.type = sp::RenderData::Type::Normal;
    screen_node->render_data.mesh = sp::MeshData::createQuad({256.0f, 224.0f}, {0.0f, 1.0f}, {1.0f, 0.0f});
    //screen_node->render_data.shader = sp::Shader::get("internal:basic.shader");
    screen_node->render_data.shader = sp::Shader::get("crt.shader");
    screen_node->render_data.texture = &screen_texture;

    sp::P<sp::SceneGraphicsLayer> screen_layer = new sp::SceneGraphicsLayer(2);
    screen_layer->addRenderPass(new UpdateScreenSizePass());
    screen_layer->addRenderPass(new sp::BasicNodeRenderPass(sc));
    window->addLayer(screen_layer);

    initTileInfo();
    UnitInfo::init();

    new sp::audio::MusicPlayer("music");
    new IngameMenuScene();
    openMainMenu();

    engine->run();

    return 0;
}
