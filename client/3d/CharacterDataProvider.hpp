#pragma once

#include <limits>
#include <DxLib.h>
#include "../ResourceManager.hpp"

#undef max

// 特定のキャラクタについてのデータを提供する
// 将来的にはsetterをなくし、ネットワークコマンドを解析する機能をつけたらいいかもしれない
// このクラスは意図的にpImplイディオムを使っていない
class CharacterDataProvider
{
public:
    CharacterDataProvider()
        : id_(std::numeric_limits<unsigned int>::max()),
          position_(VGet(0, 0, 0)),
          theta_(0),
		  vy_(0),
          motion_(-1)
    {}

    // character id
    unsigned int id() const { return id_; }
    void set_id(unsigned int val) { id_ = val; }

    // character position
    const VECTOR& position() const { return position_; }
    void set_position(const VECTOR& val) { position_ = val; }

    // character target position
    const VECTOR& target_position() const { return target_position_; }
    void set_target_position(const VECTOR& val) { target_position_ = val; }

    // character direction
    float theta() const { return theta_; }
    void set_theta(float val) { theta_ = val; }

    float vy() const { return vy_; }
    void set_vy(float val) { vy_ = val; }

    // character model number
    const ModelHandle& model() const { return model_; }
    void set_model(ModelHandle val) { model_ = val; }

    // character motion number
    int motion() const { return motion_; }
    void set_motion(int val) { motion_ = val; }

	std::string json() const { return json_; }
	void set_json(std::string json) { json_ = json;}

private:
    unsigned int id_;
    VECTOR position_;
    VECTOR target_position_;
    float theta_;
	float vy_;
    ModelHandle model_;
    int motion_;

	std::string json_;
};

