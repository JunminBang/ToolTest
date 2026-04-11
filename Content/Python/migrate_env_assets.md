# migrate_env_assets.py

외주사 납품 에셋을 프로젝트 표준 경로로 마이그레이션하는 스크립트.

---

## 목적

외주사가 `/Game/Developers/{외주사ID}` 경로에 납품한 에셋을  
프로젝트 표준 경로 `/Game/Env/{Mesh, Material, Texture}` 로 이동하고,  
머티리얼 파라미터를 프로젝트 파라미터명으로 변환한다.

원본 에셋은 삭제하지 않고 보존한다.

---

## 실행 방법

**MCP execute_python (권장)**

```python
exec(open(r'C:/Unreal/ToolTest/tools/scripts/migrate_env_assets.py', encoding='utf-8').read())
```

**언리얼 에디터 Python 콘솔**

```python
exec(open(r'C:/Unreal/ToolTest/tools/scripts/migrate_env_assets.py', encoding='utf-8').read())
```

---

## 동작 순서

| 단계 | 내용 |
|------|------|
| 1단계 | 소스 경로 에셋 스캔 → 타입 분류 (Mesh / MaterialInstance / Material / Texture) |
| 2단계 | 타입별 대상 경로로 복사 (이미 존재하면 스킵) |
| 3단계 | 복사된 MaterialInstance의 Parent를 마스터 머티리얼로 교체 |
| 4단계 | 파라미터 값 이전 (외주사 파라미터명 → 프로젝트 파라미터명) |
| 5단계 | 메시 머티리얼 슬롯을 새 경로의 머티리얼로 재연결 |
| 6단계 | 대상 경로 전체 저장 |

---

## 설정값 (스크립트 상단)

```python
SOURCE_PATH     = "/Game/Developers/qkdwnsals22"   # 외주사 납품 경로
TARGET_MESH     = "/Game/Env/Mesh"
TARGET_MATERIAL = "/Game/Env/Material"
TARGET_TEXTURE  = "/Game/Env/Texture"
MASTER_MATERIAL = "/Game/Material/M_Env_Base"       # 교체할 마스터 머티리얼
```

외주사가 바뀔 경우 `SOURCE_PATH`만 수정한다.  
마스터 머티리얼 경로가 없으면 3~4단계를 건너뛰고 경고 로그를 출력한다.

---

## 파라미터 매핑

마스터 머티리얼(`M_Env_Base`) 파라미터명을 기준으로 자동 매핑된다.

**자동 매핑 우선순위:**

| 우선순위 | 조건 | 동작 |
|----------|------|------|
| 1 | 소스 파라미터명과 마스터 파라미터명이 동일 | 그대로 연결 |
| 2 | `PARAM_MAP`에 이름 변환이 정의되어 있음 | 변환 후 연결 |
| - | 두 조건 모두 해당 없음 | 해당 파라미터 스킵 |

**현재 PARAM_MAP (이름이 다른 경우만 정의):**

| 외주사 파라미터명 | 프로젝트 파라미터명 | 타입 |
|------------------|-------------------|------|
| DiffuseColorMap  | BaseColor         | Texture |
| EmissiveColorMap | Emissive          | Texture |
| NormalMap        | Normal            | Texture |
| DiffuseColor     | BaseColorTint     | Vector |

외주사 파라미터명이 마스터와 다를 때만 `PARAM_MAP`에 추가한다.

---

## 주의사항

- 동일 이름의 에셋이 대상 경로에 이미 존재하면 복사를 스킵한다 (덮어쓰지 않음)
- 마스터 머티리얼(`MASTER_MATERIAL`)이 존재하지 않으면 파라미터 이전 없이 단순 복사만 수행된다
- 소스 경로 에셋은 삭제되지 않으므로 확인 후 수동 삭제한다
