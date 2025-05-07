# VALORANT
## MCP기반 AI Agent & AI Tools를 활용한 FPS 게임 개발 워크플로우 개선 프로젝트
### 역할 구성
- **Unreal Engine** : 이충헌, 최연택, 김병무, 김희연
- **AI Engineer** : 김형섭, 이성복, 김형후

## 1. 프로젝트 개요
AI 엔지니어 3명과 언리얼 엔진 개발자 4명이 협업하여 개발하는 온라인 슈팅(FPS) 게임 프로젝트입니다.
우리는 해당 프로젝트를 통해 게임 개발 프로세스를 개선하고 자원을 효율적으로 사용하며, 게임 내 AI 기능으로 사용자들이 비슷한 게임과는 다른 경험을 할 수 있도록 집중하고 있습니다.
- **언리얼 엔진**: 고품질 렌더링과 물리 기반 시뮬레이션을 통해 사실감 있는 FPS 환경을 구현  
- **AI 모듈**: 게임 플레이 전반에 걸쳐 다양한 AI 기술을 적용하여 개발 생산성과 사용자 경험을 극대화
- <a href="https://www.canva.com/design/DAGkCPbAfNc/c40uV0PrsDDlicrbpeQi0A/view?utm_content=DAGkCPbAfNc&utm_campaign=designshare&utm_medium=link2&utm_source=uniquelinks&utlId=h9cf42344a4">프로젝트 기획안 발표 자료</a>

## 2. 주요 AI 기능
### **무기 반동 궤적 생성 (Weapon Recoil Generation)**  
   - 강화학습 기반 모델로 실제 무기별(권총, 기관총, 산탄총) 반동 패턴 자동 생성  
   - NumPy·PyTorch를 활용해 좌표 시퀀스를 합성하고, Matplotlib으로 시각화

### **실시간 캐릭터 포즈 분석 (Pose Detection & Correction)**  
   - YOLO-Pose / OpenPose / HRNet 모델을 이용한 프레임 단위 관절 키포인트 추출  
   - LLM과 연동하여 애니메이션 블렌딩 및 자세 교정 피드백

### **절차적 콘텐츠 생성 (Procedural Content Generation)**  
   - LLM(RAG) 기반 임무 시나리오 및 대화 스크립트 자동 작성  
   - ChromaDB를 통한 사전 수집 데이터 임베딩 및 LangChain 파이프라인으로 질의-응답 구현

### **AI 기반 적군 행동 트리 (Behavior Tree AI)**  
   - 강화학습 및 휴리스틱 결합 방식으로 적군 유닛의 상황별 의사결정  
   - FastAPI 서버로 분리 운영하여 언리얼 엔진 플러그인에서 REST 호출

## 3. 기술 스택
### **프레임워크 & 라이브러리**
- PyTorch, TensorFlow, OpenCV, LangChain, ChromaDB, FastAPI
### **언리얼 엔진**
- Unreal Engine 5 (C++ / Blueprint)  
### **데이터베이스**
- SQLite (로컬 RAG), Redis
### **협업 및 배포**
- GitHub Actions, Docker, Smithery 클라우드

## 4. 게임 개발자 & 게임 사용자 활용 시나리오
### 게임 개발자 시나리오

### 게임 사용자 시나리오

## 5. 설치 및 실행 방법
1. 레포지토리 클론  
   ```bash
   git clone (https://github.com/chungheonLee0325/VALORANT.git)
   cd ai-ue-game-suite
