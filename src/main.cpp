#include <QApplication>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QPainter>
#include <QTimer>
#include <memory>

#include "first_level.hpp"
#include "game.hpp"
#include "game_level.hpp"
#include "mario.hpp"
#include "qt_game_map.hpp"
#include "qt_ui_factory.hpp"

class GameWindow : public QGraphicsView {
public:
    explicit GameWindow(QWidget* parent = nullptr)
        : QGraphicsView(parent) {
        setRenderHint(QPainter::Antialiasing, true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFocusPolicy(Qt::StrongFocus);

        game = std::make_unique<biv::Game>();
        ui_factory = std::make_unique<biv::QtUIFactory>(game.get());
        game_level = std::make_unique<biv::FirstLevel>(ui_factory.get());
        game_map = static_cast<biv::QtGameMap*>(ui_factory->get_game_map());
        mario = ui_factory->get_mario();

        setScene(game_map);
        setSceneRect(game_map->sceneRect());
        setFixedSize(sceneRect().size().toSize());

        connect(&timer, &QTimer::timeout, this, &GameWindow::tick);
        timer.start(16); // ~60 FPS
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        if (event->isAutoRepeat()) {
            return;
        }
        switch (event->key()) {
            case Qt::Key_A:
            case Qt::Key_Left:
                move_left = true;
                break;
            case Qt::Key_D:
            case Qt::Key_Right:
                move_right = true;
                break;
            case Qt::Key_Space:
            case Qt::Key_W:
            case Qt::Key_Up:
                jump_requested = true;
                break;
            case Qt::Key_Escape:
                game->finish();
                close();
                break;
            default:
                QGraphicsView::keyPressEvent(event);
        }
    }

    void keyReleaseEvent(QKeyEvent* event) override {
        if (event->isAutoRepeat()) {
            return;
        }
        switch (event->key()) {
            case Qt::Key_A:
            case Qt::Key_Left:
                move_left = false;
                break;
            case Qt::Key_D:
            case Qt::Key_Right:
                move_right = false;
                break;
            case Qt::Key_Space:
            case Qt::Key_W:
            case Qt::Key_Up:
                // Jump is handled on press.
                break;
            default:
                QGraphicsView::keyReleaseEvent(event);
        }
    }

private:
    void tick() {
        if (game->is_finished()) {
            close();
            return;
        }

        handle_input();

        // Update game state
        game->move_objs_horizontally();
        game->check_horizontally_static_collisions();

        game->move_objs_vertically();
        game->check_mario_collision();
        game->check_vertically_static_collisions();

        if (game_map->is_below_map(mario->get_top()) || !mario->is_active()) {
            game_level->restart();
            mario = ui_factory->get_mario();
        }

        if (game->is_level_end()) {
            if (!game_level->is_final()) {
                game_level.reset(game_level->get_next());
                mario = ui_factory->get_mario();
                game->start_level();
            } else {
                game->finish();
                close();
            }
        }

        // Refresh screen
        game_map->refresh();
    }

    void handle_input() {
        if (jump_requested) {
            mario->jump();
            jump_requested = false;
        }

        if (move_left && !move_right) {
            mario->move_map_left();
            if (!game->check_static_collisions(mario)) {
                game->move_map_left();
            }
            mario->move_map_right();
        } else if (move_right && !move_left) {
            mario->move_map_right();
            if (!game->check_static_collisions(mario)) {
                game->move_map_right();
            }
            mario->move_map_left();
        }
    }

private:
    std::unique_ptr<biv::Game> game;
    std::unique_ptr<biv::UIFactory> ui_factory;
    std::unique_ptr<biv::GameLevel> game_level;
    biv::QtGameMap* game_map = nullptr;
    biv::Mario* mario = nullptr;

    bool move_left = false;
    bool move_right = false;
    bool jump_requested = false;

    QTimer timer;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    GameWindow window;
    window.show();

    return QApplication::exec();
}
