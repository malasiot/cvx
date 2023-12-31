#include <cvx/viz/renderer/renderer.hpp>
#include <cvx/viz/scene/camera.hpp>
#include <cvx/viz/scene/light.hpp>
#include <cvx/viz/scene/node.hpp>
#include <cvx/viz/scene/marker.hpp>
#include <cvx/viz/scene/material.hpp>
#include <cvx/viz/scene/geometry.hpp>

#include <cvx/viz/robot/robot_scene.hpp>

#include <cvx/util/math/rng.hpp>
#include <cvx/util/misc/format.hpp>

#include <cvx/viz/physics/kinematic.hpp>

#include <iostream>
#include <thread>
#include <cvx/viz/physics/world.hpp>

#include "bullet_gui.hpp"
#include <cvx/viz/physics/multi_body.hpp>

#include <QApplication>
#include <QMainWindow>

#include <cvx/viz/image/widget.hpp>

#include <chrono>

using namespace cvx::viz ;
using namespace cvx::util ;

using namespace std ;
using namespace Eigen ;

RNG g_rng ;

struct Environment {

    struct State {
        float pole_theta_, pole_vel_ ;
        float cart_pos_, cart_vel_ ;
    };

    enum Action {
        Left = 0 , Right = 1
    };

    CameraPtr cam_ ;
    unique_ptr<OffscreenRenderer> renderer_ ;

    Environment() {
        // create a camera
        uint width = 640, height = 480 ;
        PerspectiveCamera *pcam = new PerspectiveCamera(1, // aspect ratio
                                                        50*M_PI/180,   // fov
                                                        0.0001,        // zmin
                                                        10           // zmax
                                                        ) ;

        cam_.reset(pcam) ;
        cam_->setBgColor({1, 1, 1, 1});
        cam_->lookAt(Vector3f{0.0, 4, 0}, {0, 0, 0}, {0, 0, 1.0f}) ;
        cam_->setViewport(width, height)  ;

        // create the offscreen window

        renderer_.reset(new OffscreenRenderer(width, height)) ;
        renderer_->setCamera(cam_) ;

    }

    void reset() {

        physics_.reset(new PhysicsWorld) ;
        physics_->createMultiBodyDynamicsWorld();
        physics_->setGravity({0, 0, -10});

        scene_.reset(new Scene) ;

        string path = "/home/malasiot/local/bullet3/examples/pybullet/gym/pybullet_data/cartpole.urdf" ;

        auto robot = urdf::Robot::load(path, {}, true) ;

        RobotScenePtr rs = RobotScene::fromURDF(robot) ;

        DirectionalLight *dl = new DirectionalLight(Vector3f(0.5, 0.5, 1)) ;
        dl->diffuse_color_ = Vector3f(1, 1, 1) ;
        scene_->addLight(LightPtr(dl)) ;

        scene_->addChild(rs) ;

        MultiBodyPtr body(new MultiBody) ;
        body->loadURDF(robot) ;

        body_idx_ = physics_->addMultiBody(body);


        jcp_ = body->findJoint("cart_to_pole") ;
        jsc_ = body->findJoint("slider_to_cart") ;
        jcp_->setMotorMaxImpulse(0.0) ;
        jsc_->setMotorMaxImpulse(0.0) ;
        jcp_->setDamping(0.0) ;
        jsc_->setDamping(0.0) ;
        jcp_->setFriction(0.0) ;
        jsc_->setFriction(0.0) ;


        vector<float> rp(4) ;
        g_rng.uniform(rp, -0.05f, 0.05f) ;

        jcp_->setPosition(rp[0]) ;
        jcp_->setVelocity(rp[1]) ;
        jsc_->setPosition(rp[2]) ;
        jsc_->setVelocity(rp[3]) ;
        jsc_->setTorque(0.0);

    }

    State getState() {
        State s ;
        s.pole_theta_ = jcp_->getPosition() ;
        s.pole_vel_ = jcp_->getVelocity() ;
        s.cart_pos_ = jsc_->getPosition() ;
        s.cart_vel_ = jsc_->getVelocity() ;
        return s;
    }

    void updateScene() {
        map<string, Isometry3f> transforms ;
        MultiBodyPtr b = physics_->getMultiBody(body_idx_) ;
        b->getLinkTransforms(transforms) ;
        scene_->updateTransforms(transforms) ;
    }

    cv::Mat render() {

        updateScene() ;


        renderer_->render(scene_) ;

        // obtain the color buffer
        cv::Mat clr = renderer_->getColor(true) ;

        //cv::imwrite("/tmp/oo.png", clr) ;

        return clr ;

    }

    const float action_force = 1.f ;
    const float time_step = 0.02f ;
    const float x_threshold = 0.4f ;
    const float theta_threshold_radians = 12 * 2 * M_PI / 360;
    const float linear_velocity_limit = 10.f ;
    const float angular_velocity_limit = 10.f ;
    const int pos_bins = 10 ;
    const int theta_bins = 10 ;
    const float linear_velocity_bins = 10 ;
    const float angular_velocity_bins = 10 ;

    void step(int a, State &new_state, float &reward, bool &done) {
        float force ;
        switch ( a ) {
        case Action::Left:
            force = -action_force ;
            break ;
        case Action::Right:
            force = action_force ;
        }

        jcp_->setTorque(force) ;
        physics_->stepSimulation(time_step) ;

        new_state = getState() ;
        done =  new_state.cart_pos_ < -x_threshold || new_state.cart_pos_ > x_threshold ||
                       new_state.pole_theta_ < -theta_threshold_radians ||
                       new_state.pole_theta_ > theta_threshold_radians;

        reward = 1.0f;
    }

    uint32_t quantizeState(const State &s) {
        uchar cpos_bin = floor(pos_bins * (s.cart_pos_ - ( -x_threshold ) )/(2 * x_threshold)) ;
        uchar cvel_bin = floor(linear_velocity_bins * ( s.cart_vel_ - ( -linear_velocity_limit )/(2 * linear_velocity_limit))) ;
        uchar ptheta_bin = floor(theta_bins * (s.pole_theta_ - ( -theta_threshold_radians ) )/(2 * theta_threshold_radians)) ;
        uchar pvel_bin = floor(angular_velocity_bins * ( s.pole_vel_ - ( -angular_velocity_limit )/(2 * angular_velocity_limit)));

       return ((cpos_bin << 24) | (cvel_bin << 16) | (ptheta_bin << 8) | pvel_bin ) ;
    }

    int numActions() const { return 2 ; }

    std::unique_ptr<PhysicsWorld> physics_ ;
    ScenePtr scene_{new Scene()} ;
    uint body_idx_ ;
    Joint *jcp_, *jsc_ ;
};

struct Agent {

    Agent(Environment &env): env_(env) {}

    float getQValue(uint32_t state, int action) {
        auto it = Q_.find(state) ;
        if ( it == Q_.end() ) return 0.0 ;
        auto &p = it->second ;
        if ( action >= p.size() ) return 0.0 ;
        return p[action] ;
    }

    void setQValue(uint32_t state, int action, float val) {
        auto it = Q_.find(state) ;
        if ( it == Q_.end() )
            Q_[state].resize(env_.numActions(), 0.0f) ;
        Q_[state][action] = val ;
    }

    void train(int num_episodes, float alpha, float epsilon, float gamma) {

        int end_epsilon_decay = num_episodes/2 ;
        float epsilon_decay_step = epsilon/num_episodes ;

        for ( uint ep = 0 ; ep<num_episodes ; ep++ ) {
            env_.reset() ;
            Environment::State state = env_.getState() ;
            bool done = false ;
            float reward ;
            uint step ;



            float avg_reward = 0.0 ;

            for( step = 0 ; step < 200 && !done ; step ++ ) {

                uint32_t q_state = env_.quantizeState(state) ;

                          int action = chooseAction(q_state, epsilon) ;

                env_.step(action, state, reward, done) ;


                auto it = Q_.find(q_state) ;
                if ( it == Q_.end() ) {
                    Q_[q_state].resize(env_.numActions(), 0.0) ;
                    Q_[q_state][action] = reward ;
                } else {

                    uint32_t new_q_state = env_.quantizeState(state);


                    float old_value = getQValue(q_state, action) ;

                    float next_max = getMaxQ(new_q_state) ;

                    float new_value = (1 - alpha) * old_value + alpha * (reward + gamma * next_max) ;
                    setQValue(q_state, action, new_value) ;
                }

                if ( ep % callback_freq_ == 0 ) {
                    callback_() ;
                }

                avg_reward += reward ;

            }

        //    if ( ep <= end_epsilon_decay)
         //       epsilon -= epsilon_decay_step ;

            cout << ep << ' ' << step << endl ;

        }
    }

    void setCallback(uint freq, std::function<void()> callback) {
        callback_ = callback ;
        callback_freq_ = freq ;
    }

    int chooseAction(uint32_t state, float epsilon) {

        auto it = Q_.find(state) ;

        if ( it == Q_.end() || g_rng.uniform<float>() < epsilon ) {
            return g_rng.uniform(0, env_.numActions()-1) ;
        }
        else {
            assert(it->second.size() == 2) ;
            auto ele = std::max_element(begin(it->second), end(it->second));
            int action = std::distance(begin(it->second), ele);
            return action ;
        }

    }


    float getMaxQ(uint32_t state) {
        auto it = Q_.find(state) ;
        if ( it == Q_.end() ) return 0.0 ;
        const auto &p = it->second ;
        return *std::max_element(p.begin(), p.end());

    }

    void updateQ(uint32_t state, int action,
                 float prev_reward, uint32_t prev_state, int prev_action, float alpha, float gamma) {
        Q_[prev_state][prev_action] = (1 - alpha) *
                   Q_[prev_state][prev_action] + alpha *
                   (prev_reward + (gamma * getMaxQ(state)));
    }

    map<uint32_t, vector<float>> Q_ ;
    float prev_reward_ ;
    float alpha_, gamma_ ;
    uint32_t prev_q_state_ ;
    int prev_action_ ;
    std::function<void()> callback_ ;
    uint callback_freq_ = 10 ;
    Environment &env_ ;
};


int main(int argc, char **argv)
{

    QApplication app(argc, argv) ;
    QMainWindow win ;

    QImageWidget *iw = new QImageWidget(&win) ;
    win.setCentralWidget(iw) ;
    win.resize(800, 600) ;
    win.show() ;

    thread t( [&]{
        Environment env ;
        Agent agent(env) ;

        agent.setCallback(500, [&] {
            auto im = env.render() ;
            QMetaObject::invokeMethod(iw, "setImage", Qt::QueuedConnection, Q_ARG(cv::Mat, im) );
        });

        agent.train(10000, 0.9, 0.1, 0.9);
    }) ;


    app.exec() ;

}
