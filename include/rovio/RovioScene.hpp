/*
* Copyright (c) 2014, Autonomous Systems Lab
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* * Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of the Autonomous Systems Lab, ETH Zurich nor the
* names of its contributors may be used to endorse or promote products
* derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#ifndef ROVIO_ROVIOSCENE_HPP_
#define ROVIO_ROVIOSCENE_HPP_

#include "kindr/rotations/RotationEigen.hpp"
#include <Eigen/Dense>

#include "commonVision.hpp"
#include "rovio/Scene.hpp"

namespace rot = kindr::rotations::eigen_impl;

namespace rovio {

template<typename FILTER>
class RovioScene{
 public:
  typedef typename FILTER::mtFilterState mtFilterState;
  typedef typename mtFilterState::mtState mtState;
  FILTER* mpFilter_;

  rovio::Scene mScene;
  SceneObject* mpSensor_ = nullptr;
  SceneObject* mpLines_ = nullptr;
  SceneObject* mpDepthVar_ = nullptr;
  SceneObject* mpPatches_[mtState::nMax_];
  void initScene(int argc, char** argv, const std::string& mVSFileName,const std::string& mFSFileName,FILTER* mpFilter){
    initGlut(argc,argv,mScene);
    mScene.init(argc, argv,mVSFileName,mFSFileName);
    mpFilter_ = mpFilter;

    rovio::SceneObject* mpGroundplane1 = mScene.addSceneObject();
    mpGroundplane1->makeGroundPlaneMesh(0.25,40);
    mpGroundplane1->setColorFull(Eigen::Vector4f(0.6f,0.6f,0.6f,1.0f));
    mpGroundplane1->lineWidth_ = 1.0f;
    mpGroundplane1->W_r_WB_(2) = -1.0f;
    rovio::SceneObject* mpGroundplane2 = mScene.addSceneObject();
    mpGroundplane2->makeGroundPlaneMesh(1.0,10);
    mpGroundplane2->setColorFull(Eigen::Vector4f(0.8f,0.8f,0.8f,1.0f));
    mpGroundplane2->lineWidth_ = 3.0f;
    mpGroundplane2->W_r_WB_(2) = -1.0f;
    mpSensor_ = mScene.addSceneObject();
    mpSensor_->makeCoordinateFrame(1.0f);
    mpSensor_->lineWidth_ = 5.0f;

    for(int i=0;i<mtState::nMax_;i++){
      mpPatches_[i] = mScene.addSceneObject();
    }

    mpLines_ = mScene.addSceneObject();
    mpLines_->makeLine();
    mpLines_->lineWidth_ = 2.0f;
    mpLines_->mode_ = GL_LINES;

    mpDepthVar_ = mScene.addSceneObject();
    mpDepthVar_->makeLine();
    mpDepthVar_->lineWidth_ = 3.0f;
    mpDepthVar_->mode_ = GL_LINES;

    mScene.setView(Eigen::Vector3f(-5.0f,-5.0f,5.0f),Eigen::Vector3f(0.0f,0.0f,0.0f));
    mScene.setYDown();
  }
  void setIdleFunction(void (*idleFunc)()){
    mScene.setIdleFunction(idleFunc);
  }
  void drawScene(mtFilterState& filterState){
    const mtState& state = filterState.state_;
    const typename mtFilterState::mtFilterCovMat& cov = filterState.cov_;

    mpSensor_->W_r_WB_ = state.get_WrWB().template cast<float>();
    mpSensor_->q_BW_ = state.get_qBW();

    std::vector<Eigen::Vector3f> points;
    std::vector<Eigen::Vector3f> lines;
    mpLines_->clear();
    mpDepthVar_->clear();
    double d, d_far, d_near, d_p, p_d, p_d_p;
    for(unsigned int i=0;i<mtState::nMax_;i++){
      if(filterState.mlps_.isValid_[i]){
        state.template get<mtState::_aux>().depthMap_.map(filterState.state_.template get<mtState::_dep>(i),d,d_p,p_d,p_d_p);
        const double sigma = cov(mtState::template getId<mtState::_dep>(i),mtState::template getId<mtState::_dep>(i));
        state.template get<mtState::_aux>().depthMap_.map(filterState.state_.template get<mtState::_dep>(i)-20*sigma,d_far,d_p,p_d,p_d_p);
        if(d_far > 1000 || d_far <= 0.0) d_far = 1000;
        state.template get<mtState::_aux>().depthMap_.map(filterState.state_.template get<mtState::_dep>(i)+20*sigma,d_near,d_p,p_d,p_d_p);
        const LWF::NormalVectorElement middle = filterState.state_.template get<mtState::_nor>(i);
        LWF::NormalVectorElement corner[4];
        Eigen::Vector3d cornerVec[4];
        const BearingCorners& bearingCorners = filterState.mlps_.features_[i].get_bearingCorners();
        for(int x=0;x<2;x++){
          for(int y=0;y<2;y++){
            const Eigen::Vector2d dif = 4.0*((2*x-1)*filterState.state_.template get<mtState::_aux>().bearingCorners_[i][0]+(2*y-1)*filterState.state_.template get<mtState::_aux>().bearingCorners_[i][1]); // TODO: factor 4
            middle.boxPlus(dif,corner[y*2+x]);
            cornerVec[y*2+x] = corner[y*2+x].getVec()*d;
          }
        }
        const Eigen::Vector3d pos = middle.getVec()*d;
        const Eigen::Vector3d pos_far = middle.getVec()*d_far;
        const Eigen::Vector3d pos_near = middle.getVec()*d_near;

        mpLines_->prolonge(cornerVec[0].cast<float>());
        mpLines_->prolonge(cornerVec[1].cast<float>());
        mpLines_->prolonge(cornerVec[1].cast<float>());
        mpLines_->prolonge(cornerVec[3].cast<float>());
        mpLines_->prolonge(cornerVec[3].cast<float>());
        mpLines_->prolonge(cornerVec[2].cast<float>());
        mpLines_->prolonge(cornerVec[2].cast<float>());
        mpLines_->prolonge(cornerVec[0].cast<float>());

        mpDepthVar_->prolonge(pos_far.cast<float>());
        mpDepthVar_->prolonge(pos_near.cast<float>());
        if(filterState.mlps_.features_[i].status_.inFrame_){
          if(filterState.mlps_.features_[i].status_.trackingStatus_ == TRACKED){
            std::next(mpDepthVar_->vertices_.rbegin())->color_.fromEigen(Eigen::Vector4f(0.0f,1.0f,0.0f,1.0f));
            mpDepthVar_->vertices_.rbegin()->color_.fromEigen(Eigen::Vector4f(0.0f,1.0f,0.0f,1.0f));
            for(int j=0;j<8;j++){
              std::next(mpLines_->vertices_.rbegin(),j)->color_.fromEigen(Eigen::Vector4f(0.0f,1.0f,0.0f,1.0f));
            }
          } else {
            std::next( mpDepthVar_->vertices_.rbegin())->color_.fromEigen(Eigen::Vector4f(1.0f,0.0f,0.0f,1.0f));
            mpDepthVar_->vertices_.rbegin()->color_.fromEigen(Eigen::Vector4f(1.0f,0.0f,0.0f,1.0f));
            for(int j=0;j<8;j++){
              std::next(mpLines_->vertices_.rbegin(),j)->color_.fromEigen(Eigen::Vector4f(1.0f,0.0f,0.0f,1.0f));
            }
          }
        } else {
          std::next( mpDepthVar_->vertices_.rbegin())->color_.fromEigen(Eigen::Vector4f(0.5f,0.5f,0.5f,1.0f));
          mpDepthVar_->vertices_.rbegin()->color_.fromEigen(Eigen::Vector4f(0.5f,0.5f,0.5f,1.0f));
          for(int j=0;j<8;j++){
            std::next(mpLines_->vertices_.rbegin(),j)->color_.fromEigen(Eigen::Vector4f(0.5f,0.5f,0.5f,1.0f));
          }
        }

        mpPatches_[i]->makeTexturedRectangle(1.0f,1.0f);
        cv::Mat patch = cv::Mat::zeros(mtState::patchSize_*pow(2,mtState::nLevels_-1),mtState::patchSize_*pow(2,mtState::nLevels_-1),CV_8UC1);
        filterState.mlps_.features_[i].drawMultilevelPatch(patch,cv::Point2i(0,0),1,false);
        mpPatches_[i]->setTexture(patch);
        for(int x=0;x<2;x++){
          for(int y=0;y<2;y++){
            mpPatches_[i]->vertices_[y*2+x].pos_.fromEigen(cornerVec[y*2+x].cast<float>());
          }
        }
        mpPatches_[i]->allocateBuffer();
        mpPatches_[i]->W_r_WB_ = mpSensor_->W_r_WB_;
        mpPatches_[i]->q_BW_ = mpSensor_->q_BW_;
      } else {
        mpPatches_[i]->clear();
      }
    }
    mpLines_->W_r_WB_ = mpSensor_->W_r_WB_;
    mpLines_->q_BW_ = mpSensor_->q_BW_;
    mpLines_->allocateBuffer();
    mpDepthVar_->W_r_WB_ = mpSensor_->W_r_WB_;
    mpDepthVar_->q_BW_ = mpSensor_->q_BW_;
    mpDepthVar_->allocateBuffer();
  }
};

}


#endif /* ROVIO_ROVIOSCENE_HPP_ */