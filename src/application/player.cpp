#include "player.h"

namespace application {

Player::Player(model::GameSession* session, std::shared_ptr<model::Dog> dog) 
: session_{session}
, dog_{dog} {}

model::GameSession* Player::GetSession() {
    return session_;
}

std::shared_ptr<model::Dog> Player::GetDog() {
    return dog_;
}

model::GameSession Player::GetSession() const {
    return *session_;
}
std::shared_ptr<model::Dog> Player::GetDog() const {
    return dog_;
}

}