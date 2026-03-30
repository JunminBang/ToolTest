# ToolTest

Unreal Engine 에디터 플러그인 학습/실험용 프로젝트입니다.

---

## 플러그인 목록

| 플러그인 | 설명 |
|---|---|
| [GuideMeshViewer](#guidemeshviewer) | 스켈레탈 메시 에디터에 가이드 스태틱 메시를 오버레이 렌더링 |
| [AssetMemo](#assetmemo) | 에셋에 메모와 작업 상태를 붙이고 폴더로 관리 |
| [AnimTools](#animtools) | 스켈레탈 메시 본 조회 및 AnimBlueprint 자동 생성 유틸리티 |

---

## GuideMeshViewer

스켈레탈 메시 에디터(Persona) 뷰포트에 기준 스태틱 메시를 반투명으로 겹쳐 보여주는 플러그인입니다.
캐릭터 비율 확인, 장비 부착 위치 가이드 등에 활용할 수 있습니다.

### 기능

- 스켈레탈 메시 에디터를 열면 전용 **Guide Mesh Viewer** 탭 자동 생성
- DetailsView에서 실시간으로 설정 변경 가능
  - **Visible** — 가이드 메시 표시/숨김 토글
  - **Static Mesh** — 표시할 스태틱 메시 지정
  - **Transform** — 위치/회전/스케일 조정
  - **Color** — 머티리얼 `Color` 벡터 파라미터 전달
  - **Opacity** — 머티리얼 `Opacity` 스칼라 파라미터 전달 (0.0 ~ 1.0)
- 여러 스켈레탈 메시 에디터를 동시에 열어도 각각 독립적으로 동작
- 에디터 탭을 닫으면 뷰포트에서 자동 제거

### 구조

```
Plugins/GuideMeshViewer/Source/GuideMeshViewer/
├── GuideMeshViewerConfig.h/cpp   — UObject 설정 (UPROPERTY, PostEditChangeProperty)
└── GuideMeshViewerModule.h/cpp   — 모듈, 탭 등록, 씬 데이터 관리, 메시 스폰
```

| 클래스/구조체 | 역할 |
|---|---|
| `UGuideMeshViewerConfig` | 씬별 설정 UObject. 프로퍼티 변경 시 `OnConfigChanged` 델리게이트 브로드캐스트 |
| `FGuideMeshSceneData` | 씬별 상태 (Config, Actor, StaticMeshComponent, PreviewScene 약참조) |
| `FGuideMeshViewerModule` | 탭 등록/해제, `ApplyConfig`로 설정 변경 사항을 뷰포트에 반영 |

---

## AssetMemo

콘텐츠 브라우저의 에셋에 메모와 작업 상태를 붙이고 폴더로 분류하는 에디터 플러그인입니다.

### 기능

- **에셋 드래그&드롭** — 콘텐츠 브라우저에서 AssetMemo 창으로 드래그하여 추가
- **폴더 관리**
  - `+ 새 폴더` 버튼으로 폴더 생성
  - 폴더 행 클릭으로 접기/펼치기
  - **F2** 로 폴더 이름 변경 (클릭으로 폴더 활성화 후 F2)
  - 폴더 삭제 시 하위 에셋도 함께 제거
- **에셋 행**
  - 에셋 이름 클릭 → 콘텐츠 브라우저에서 해당 에셋으로 이동
  - **상태** 콤보박스: `미작업` / `진행중` / `완료`
  - **메모** 텍스트 입력
  - ✕ 버튼으로 목록에서 제거
- **데이터 저장** — `Saved/AssetMemo.json`에 자동 저장

### 사용 방법

1. 에디터 상단 메뉴 **Tools > Asset Memo** 또는 탭 검색으로 창 열기
2. 콘텐츠 브라우저에서 에셋을 **드래그&드롭**으로 추가
   - 폴더 위에 드롭 → 해당 폴더에 추가
   - 빈 공간에 드롭 → 첫 번째 폴더(기본 폴더)에 추가
3. 상태와 메모 입력 후 자동 저장

### 구조

```
Plugins/AssetMemo/Source/AssetMemo/
├── AssetMemoModule.h/cpp     — 모듈, 탭 등록
├── AssetMemoData.h/cpp       — 데이터 모델 (Folder/Entry), JSON 직렬화
├── SAssetMemoWidget.h/cpp    — 메인 위젯 (SCompoundWidget, 드래그&드롭)
└── AssetMemo.Build.cs
```

| 클래스/구조체 | 역할 |
|---|---|
| `FAssetMemoEntry` | 에셋 1개의 데이터 (ObjectPath, AssetName, AssetClass, Status, Memo) |
| `FAssetMemoFolder` | 폴더 1개의 데이터 (Name, bExpanded, Entries[]) |
| `FAssetMemoStorage` | JSON 파일 Load/Save |
| `FMemoTreeItem` | 트리 노드 (폴더 또는 에셋, union 구조) |
| `SAssetMemoWidget` | 메인 위젯. 트리뷰, 드래그&드롭, 폴더 CRUD 처리 |
| `SAssetMemoTreeRow` | 트리 행 위젯 (SMultiColumnTableRow). 폴더/에셋 행 구분 렌더링 |

---

## AnimTools

스켈레탈 메시의 본 존재 여부를 확인하고, 조건에 맞는 AnimBlueprint 템플릿을 자동으로 복제·설정하는 유틸리티입니다.
C++ 플러그인(`AnimTools`)과 Python 스크립트(`Content/Python/`)로 구성됩니다.

### 기능

- **HasBone** — 스켈레탈 메시의 Reference Skeleton에서 본 이름을 조회 (Python/Blueprint에서 호출 가능)
- **ABP 자동 생성** — 선택한 스켈레탈 메시에서 `skirt_1` ~ `skirt_6` 본 중 가장 높은 번호를 감지해 대응하는 템플릿 ABP를 복제
  - 복제 위치: 스켈레탈 메시와 같은 폴더
  - 네이밍: `ABP_<스켈레탈메시이름>`
  - 스켈레톤 자동 할당, 템플릿 플래그 해제, 프리뷰 메시 지정 후 저장

### 사용 방법

1. Content Browser에서 Skeletal Mesh 선택
2. Editor Utility Blueprint(EUB)에서 아래 Python 실행

```python
import importlib, create_anim_blueprint_utility
importlib.reload(create_anim_blueprint_utility)
create_anim_blueprint_utility.run()
```

| skirt 본 (최고) | 사용 템플릿 | 생성 결과 |
|---|---|---|
| `skirt_1` | `ABP_Test_Template_1` | `ABP_<메시이름>` |
| `skirt_3` | `ABP_Test_Template_3` | `ABP_<메시이름>` |
| `skirt_6` | `ABP_Test_Template_6` | `ABP_<메시이름>` |
| skirt 본 없음 | — | 생성 안 함 |

### 구조

```
Plugins/AnimTools/Source/AnimTools/
├── Public/AnimToolsBlueprintLibrary.h    — HasBone, SetupDuplicatedAnimBlueprint 선언
├── Private/AnimToolsBlueprintLibrary.cpp — GetRefSkeleton().FindBoneIndex, SetPreviewMesh 구현
├── Public/AnimToolsModule.h
├── Private/AnimToolsModule.cpp
└── AnimTools.Build.cs

Content/Python/
├── create_anim_blueprint_utility.py      — ABP 생성 메인 스크립트
└── init_unreal.py                        — 에디터 시작 시 자동 임포트
```

| 함수 | 역할 |
|---|---|
| `UAnimToolsBlueprintLibrary::HasBone` | SkeletalMesh + BoneName → bool |
| `UAnimToolsBlueprintLibrary::SetupDuplicatedAnimBlueprint` | 템플릿 플래그 해제, 프리뷰 메시 지정 |
| `create_anim_blueprint_utility.run()` | 선택 메시 순회 → 본 감지 → 복제 → 설정 → 저장 |

### 필요 플러그인 (자동 활성화)

- `PythonScriptPlugin`
- `EditorScriptingUtilities`

---

## 개발 환경

- Unreal Engine 5.7
- Windows 11
