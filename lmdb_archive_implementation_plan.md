# LMDB Archive 구현 계획서

## 1. 목표

`LMDB Archive`는 **LMDB 기반 아카이브 파일/폴더를 7-Zip처럼 탐색, 생성, 추출, 검증, 갱신**할 수 있는 크로스플랫폼 데스크톱 유틸리티이다.

이 문서는 Claude Code가 **처음부터 완성도 높은 구현**을 할 수 있도록, 요구사항을 설계 수준까지 구체화한 구현 계획서다.

---

## 2. 핵심 요구사항

### 2.1 제품 목표
- **동작과 UI는 7-Zip과 동일한 감각**을 제공한다.
- **Windows / Linux 동시 지원**을 위해 **Qt6** 기반으로 구현한다.
- 폴더를 LMDB 아카이브로 만들거나, LMDB 아카이브를 폴더로 풀 수 있어야 한다.
- 저장 규칙은 실제 폴더 구조를 key 경로 문자열로 보존한다.
  - 루트 파일: `File.jpg`
  - 하위 폴더 파일: `SubFolder/File.jpg`
  - 다중 하위 폴더: `Sub1/Sub2/File.jpg`

### 2.2 사용자 관점 기능
1. LMDB 아카이브 열기
2. LMDB 아카이브 내부를 폴더 트리처럼 탐색
3. 파일 미리보기(특히 jpg/png/bmp 등 이미지)
4. 폴더 → LMDB 아카이브 만들기
5. LMDB 아카이브 → 폴더로 추출하기
6. 선택 항목만 추출하기
7. 아카이브 내부 파일 추가 / 삭제 / 덮어쓰기
8. 무결성 검사(verify)
9. 기본 통계 보기(파일 수, 총 바이트 수, 확장자 분포 등)
10. drag & drop 지원
11. 명령행 모드도 제공하여 자동화 가능하게 하기

---

## 3. 제품 범위

### 3.1 1차 버전(MVP)
반드시 구현:
- LMDB 아카이브 생성
- LMDB 아카이브 열기
- 트리/리스트 기반 탐색 UI
- 전체 추출 / 선택 추출
- 파일 추가 / 폴더 추가
- 파일 삭제
- 기본 이미지 미리보기
- Verify
- CLI 기본 명령

### 3.2 2차 버전
- 파일 교체
- 대량 작업 진행률 상세화
- 썸네일 캐시
- 검색 / 필터 / 정렬 강화
- 아카이브 비교(diff)
- read-only / repair 모드

### 3.3 3차 버전
- 가상 파일시스템 마운트 연동 검토
- 원격 저장소 동기화 검토
- 내부 메타데이터 버전 업그레이드 툴

---

## 4. UX 원칙

### 4.1 7-Zip과 동일한 감각
완전히 7-Zip의 내부 구현을 복제하는 것이 아니라, **사용자 체감 UX와 작업 흐름을 7-Zip 수준으로 맞춘다**.

필수 요소:
- 상단 툴바
- 주소창 / 경로 이동
- 좌측 폴더 트리
- 우측 파일 리스트
- 상태바
- 더블클릭으로 폴더 진입
- 상위 폴더 이동 (`..`)
- 우클릭 컨텍스트 메뉴
- Extract / Add / Delete / Refresh / Properties / Verify

### 4.2 성능 우선 UX
- 대량 파일 목록 표시 시 lazy loading 또는 batched model refresh 고려
- 큰 이미지 미리보기는 원본 전체 decode 대신 제한 해상도 preview 사용
- 진행률 표시와 cancel 지원
- UI thread block 금지

---

## 5. 저장 모델 설계

## 5.1 LMDB 아카이브의 기본 개념
LMDB는 실제 폴더 구조를 갖지 않으므로, **key를 경로 문자열(path-like key)** 로 저장한다.

예시:
- `File.jpg`
- `SubFolder/File.jpg`
- `SubFolder/Nested/File.jpg`

value는 파일의 raw bytes이다.

즉:
- key = 논리적 파일 경로
- value = 파일 내용(binary)

### 5.2 디렉토리 표현 방식
디렉토리는 별도 실체로 저장하지 않고, **key prefix** 기반의 가상 폴더로 해석한다.

예:
- `A/B/C.jpg`
- `A/B/D.jpg`
- `A/E/F.jpg`

UI 상에서는:
- `A/`
- `A/B/`
- `A/E/`

를 디렉토리처럼 보여준다.

### 5.3 빈 폴더 처리 정책
LMDB key 기반 구조는 빈 폴더를 자동 표현할 수 없으므로 정책을 명확히 한다.

기본 정책:
- **MVP에서는 빈 폴더를 지원하지 않는다.**
- 폴더는 하위 파일이 하나 이상 있을 때만 존재한다고 본다.

확장 정책(옵션):
- 예약된 special key로 빈 폴더 메타를 저장할 수 있음
- 예: `__dir__/EmptyFolder`
- 단, MVP에서는 복잡도를 줄이기 위해 미구현

---

## 6. LMDB 내부 스키마

단순 파일 key/value만 저장하면 기능 확장이 어렵다. 따라서 **data + metadata + manifest** 구성을 사용한다.

### 6.1 DBI 구성
LMDB named database를 사용한다.

권장 DBI:
1. `entries`
   - key: 파일 경로 (`Sub/File.jpg`)
   - value: raw file bytes
2. `meta`
   - key: 파일 경로
   - value: 직렬화된 파일 메타데이터(JSON 또는 고정 바이너리 구조체)
3. `manifest`
   - key: 예약 문자열
   - value: 아카이브 전체 정보
4. `thumb_cache` (선택)
   - key: 파일 경로
   - value: 소형 preview bytes

### 6.2 manifest 내용
최소 포함:
- archive format version
- created_at
- updated_at
- tool name/version
- file_count
- total_uncompressed_bytes
- optional comment

### 6.3 file metadata 내용
최소 포함:
- relative_path
- file_size
- modified_time_utc
- created_time_utc (가능 시)
- crc32 또는 xxhash64
- mime / extension
- image_width / image_height (이미지인 경우)

### 6.4 직렬화 포맷
MVP 권장:
- 메타데이터는 **JSON** 저장
- 이유: 디버깅 용이, 타 툴 연동 용이, 사람이 읽기 쉬움

향후 최적화:
- CBOR / MessagePack / custom binary로 교체 가능

---

## 7. 파일 경로 규칙

### 7.1 경로 통일 규칙
모든 internal key는 다음 규칙을 따른다.

- 구분자는 무조건 `/`
- 절대경로 금지
- `.` 및 `..` 정규화
- 시작 `/` 금지
- 끝 `/` 금지
- Windows 백슬래시 `\\`는 `/`로 변환

예:
- `C:\\temp\\A.jpg` → `A.jpg` 또는 상대경로 기준 `Sub/A.jpg`
- `Sub\\File.jpg` → `Sub/File.jpg`

### 7.2 대소문자 정책
- 내부 key는 **원본 파일명 유지**
- 비교는 플랫폼 독립적으로 처리하되, 내부 저장은 case-sensitive로 유지
- Windows에서 충돌 가능성(`a.jpg`, `A.jpg`)은 반드시 감지 후 사용자에게 경고

### 7.3 금지 경로
다음은 거부:
- 빈 문자열
- `/` 시작 경로
- `../` 탈출 경로
- null byte 포함

---

## 8. 열기/생성/추출 동작 정의

### 8.1 폴더 → LMDB 아카이브 생성
입력:
- source directory
- target lmdb archive directory/file path

동작:
1. source directory 재귀 순회
2. regular file만 대상
3. 상대경로 계산
4. 상대경로를 internal key로 정규화
5. 파일 bytes를 `entries` DBI에 저장
6. 메타데이터를 `meta` DBI에 저장
7. manifest 갱신
8. 작업 완료 후 verify optional 수행

### 8.2 LMDB 아카이브 → 폴더 추출
입력:
- archive path
- destination directory
- 선택 prefix 또는 선택 파일 목록(optional)

동작:
1. 선택 범위 결정
2. 각 key에 대해 대상 경로 생성
3. 상위 디렉토리 생성
4. bytes 기록
5. modified time 복원(optional)
6. 검증(optional)

### 8.3 아카이브 열기
동작:
1. LMDB 환경 open read-only 또는 read-write
2. manifest 확인
3. index build(메모리 트리 생성)
4. UI 트리/리스트 갱신

---

## 9. UX 세부 동작 정의

## 9.1 메인 윈도우 레이아웃
- 메뉴바
- 툴바
- 주소창
- 좌측: 폴더 트리 (`QTreeView`)
- 우측: 파일 리스트 (`QTreeView` or `QTableView`)
- 하단: preview pane(optional splitter) 또는 별도 preview dock
- 상태바

### 9.2 메뉴 구성
#### File
- Open Archive...
- Create Archive from Folder...
- Extract...
- Close Archive
- Exit

#### Edit
- Delete
- Rename (선택)
- Select All
- Refresh

#### View
- Large Icons (선택)
- Details
- Preview Pane
- Toolbar
- Status Bar

#### Tools
- Verify Archive
- Rebuild Index (선택)
- Statistics
- Options

#### Help
- About

### 9.3 툴바 버튼
- Open
- Create
- Extract
- Add File
- Add Folder
- Delete
- Up
- Refresh
- Verify

### 9.4 우클릭 메뉴
파일/폴더 선택 시:
- Open / Preview
- Extract...
- Extract Here
- Add Files...
- Add Folder...
- Delete
- Copy Path
- Properties

---

## 10. 데이터 모델 및 인덱싱 전략

### 10.1 UI 표시용 인메모리 트리
LMDB 전체를 매번 cursor scan 하며 UI를 그리면 비효율적이므로, 아카이브 open 시 인메모리 tree model을 구성한다.

권장 구조:
- `Node`
  - name
  - full_path
  - is_dir
  - children
  - file metadata(optional)

### 10.2 트리 빌드 방식
모든 key를 순회하며 `/` 기준 split 후 트리 구성.

예:
- `A/B/C.jpg`
- `A/B/D.jpg`
- `A/E/F.jpg`

트리 생성 결과:
- A
  - B
    - C.jpg
    - D.jpg
  - E
    - F.jpg

### 10.3 리스트 뷰 모델
현재 폴더 path 기준으로 직접 하위 자식만 표시한다.

컬럼:
- Name
- Size
- Type
- Modified
- Path(optional)

### 10.4 검색
MVP 옵션:
- 현재 폴더 내 이름 검색
- 전체 key substring 검색

확장:
- wildcard / regex / extension filter

---

## 11. 이미지 미리보기

### 11.1 지원 포맷
Qt가 기본 지원하는 이미지 포맷부터 사용:
- jpg/jpeg
- png
- bmp
- gif(정적)
- webp(플랫폼 설정에 따라)

### 11.2 동작
1. 사용자가 이미지 파일 선택
2. LMDB에서 bytes read
3. `QImage::loadFromData()` 사용
4. UI preview 표시
5. 큰 이미지인 경우 fit-to-view

### 11.3 최적화
- preview는 worker thread에서 decode
- 연속 선택 시 이전 decode 취소 또는 outdated discard
- thumbnail cache는 2차 버전 고려

---

## 12. 쓰기 동작 정책

### 12.1 Add File
- 로컬 파일 선택
- 현재 폴더 기준 또는 사용자 지정 경로로 추가
- 동일 key 존재 시 overwrite confirm

### 12.2 Add Folder
- 폴더 재귀 스캔 후 현재 위치에 병합
- overwrite / skip / rename 규칙 제공

### 12.3 Delete
- 파일 선택 삭제
- 폴더 선택 시 prefix 전체 삭제
- confirm dialog 필수

### 12.4 Rename
MVP에서 파일 rename만 우선 고려.

주의:
- LMDB는 key rename이 곧 **새 key put + 기존 key delete**
- 디렉토리 rename은 prefix 전체 재작성 필요하므로 2차 기능으로 미룸

---

## 13. Verify / Repair 정책

### 13.1 Verify
검사 항목:
- manifest 존재 여부
- required DBI 존재 여부
- 각 `entries` key에 대응하는 `meta` 존재 여부
- file_size 일치 여부
- hash 일치 여부(optional)
- path normalization 위반 여부
- duplicate / invalid key 여부

### 13.2 Repair
MVP에서는 자동 repair를 적극적으로 하지 말고, **진단 리포트 생성** 수준으로 시작한다.

예:
- orphan entry
- missing meta
- invalid path
- corrupted meta JSON

2차 버전에서 선택적 repair 지원 검토.

---

## 14. CLI 설계

GUI만 있으면 자동화와 디버그에 약하므로 CLI를 같이 제공한다.

실행 파일 예:
- `lmdb-archive`

### 14.1 기본 명령
- `create <folder> <archive>`
- `extract <archive> <folder>`
- `list <archive> [prefix]`
- `verify <archive>`
- `stat <archive>`
- `add <archive> <file_or_folder> [dest_prefix]`
- `delete <archive> <path_or_prefix>`
- `cat <archive> <internal_path> <output_file>`

### 14.2 예시
```bash
lmdb-archive create ./input ./defects.lmdb
lmdb-archive list ./defects.lmdb SubFolder/
lmdb-archive extract ./defects.lmdb ./out
lmdb-archive verify ./defects.lmdb
lmdb-archive cat ./defects.lmdb SubFolder/File.jpg ./File.jpg
```

---

## 15. 프로젝트 구조 제안

```text
LMDBArchive/
  CMakeLists.txt
  README.md
  docs/
    lmdb_archive_implementation_plan.md
  src/
    app/
      main.cpp
      AppController.h/.cpp
    core/
      ArchiveTypes.h
      PathUtil.h/.cpp
      FileSystemUtil.h/.cpp
      HashUtil.h/.cpp
      MetaCodec.h/.cpp
    lmdb/
      LmdbEnv.h/.cpp
      LmdbArchive.h/.cpp
      LmdbCursor.h/.cpp
    model/
      ArchiveTreeNode.h/.cpp
      ArchiveTreeBuilder.h/.cpp
      FolderTreeModel.h/.cpp
      EntryTableModel.h/.cpp
    ui/
      MainWindow.h/.cpp
      PreviewPane.h/.cpp
      ExtractDialog.h/.cpp
      ProgressDialog.h/.cpp
      PropertiesDialog.h/.cpp
    worker/
      CreateArchiveWorker.h/.cpp
      ExtractWorker.h/.cpp
      VerifyWorker.h/.cpp
      PreviewDecodeWorker.h/.cpp
    cli/
      CliMain.cpp
      CliCommands.h/.cpp
  tests/
    test_pathutil.cpp
    test_archive_roundtrip.cpp
    test_treebuilder.cpp
    test_verify.cpp
```

---

## 16. 기술 스택

### 16.1 필수
- C++20
- Qt6
  - Core
  - Gui
  - Widgets
  - Concurrent(optional)
- LMDB C library
- CMake

### 16.2 권장
- fmt
- spdlog
- Catch2 또는 GoogleTest
- xxHash 또는 CRC32 구현

### 16.3 플랫폼
- Windows 10/11
- Linux (Ubuntu 계열 우선)

---

## 17. LMDB 추상화 레이어 설계

raw LMDB API를 UI 레이어까지 노출하지 말고 wrapper를 만든다.

### 17.1 핵심 클래스
#### `LmdbEnv`
책임:
- environment 생성 / open / close
- map size 설정
- DBI open
- read-only / read-write 모드 관리

#### `LmdbArchive`
책임:
- create/open
- list entries
- read entry
- write entry
- delete entry
- load manifest
- save manifest
- verify

#### `ArchiveEntry`
필드:
- path
- size
- modified_time
- hash
- is_image
- image_width
- image_height

### 17.2 트랜잭션 정책
- 읽기: 짧은 read txn
- 쓰기: 작업 단위별 write txn
- 대량 import 시 batch commit 허용

### 17.3 map size 정책
초기 생성 시 넉넉한 기본값 제공.
예:
- 기본 4GB 또는 16GB
- 옵션에서 변경 가능
- 부족 시 resize 지원

---

## 18. 파일 포맷/배포 형태 결정

### 18.1 LMDB를 “폴더”로 취급할지 “파일”로 취급할지
LMDB 기본 구조는 보통 다음 파일들로 구성된다.
- `data.mdb`
- `lock.mdb`

따라서 사용자 관점 포맷은 두 가지 선택지가 있다.

#### 옵션 A: `.lmdb/` 디렉토리
예:
- `review_archive.lmdb/`
  - `data.mdb`
  - `lock.mdb`

장점:
- LMDB의 기본 구조와 자연스럽다
- 구현 단순

단점:
- 사용자가 파일 하나처럼 느끼지 못한다

#### 옵션 B: `.lmdbarchive/` 디렉토리
LMDB 내부 파일을 감춘 product-specific layout
예:
- `review_archive.lmdbarchive/`
  - `db/data.mdb`
  - `db/lock.mdb`
  - `manifest.json` (선택)

장점:
- 제품 identity 명확
- 확장 파일 추가 쉬움

단점:
- 약간 더 복잡

**권장: 옵션 A 또는 B 중 하나를 일관되게 채택하라.**
MVP에서는 구현 단순성을 위해 `*.lmdb/` 디렉토리 방식을 추천한다.

---

## 19. 작업 흐름 상세

### 19.1 Open Archive
1. 사용자 선택
2. 유효성 검사
3. LMDB env open
4. manifest 읽기
5. 전체 key scan
6. 트리 모델 생성
7. UI 연결

### 19.2 Create Archive
1. 폴더 선택
2. 타깃 archive 경로 선택
3. 옵션 설정 (overwrite, verify after create)
4. worker 시작
5. 파일 스캔 + 저장 + 메타 기록
6. progress 업데이트
7. 완료 후 archive open

### 19.3 Extract
1. 현재 선택 항목 또는 전체 범위 계산
2. 대상 폴더 선택
3. 충돌 정책 선택
4. worker 실행
5. bytes 기록
6. 결과 리포트 출력

---

## 20. 테스트 계획

### 20.1 단위 테스트
- 경로 정규화
- path split/join
- invalid path rejection
- metadata serialize/deserialize
- tree builder correctness
- overwrite policy

### 20.2 통합 테스트
- 폴더 → 아카이브 → 폴더 round-trip
- 이미지 preview 가능 여부
- 1만 파일 수준 create/list/extract
- nested folder 처리
- 한글/공백/특수문자 파일명 처리

### 20.3 플랫폼 테스트
- Windows 경로 처리
- Linux 경로 처리
- UTF-8 파일명
- 대소문자 충돌 시나리오

### 20.4 장애 테스트
- 중간 취소
- 파일 접근 권한 오류
- 잘못된 meta JSON
- map size 부족
- 매우 긴 경로

---

## 21. 성능 목표

### 21.1 목표
- 10만 파일 목록 로딩 시 수 초 내 usable 상태
- 이미지 단건 preview 즉시성 확보
- extract/create 동안 UI block 없음

### 21.2 최적화 포인트
- open 시 full scan 후 tree build
- list view는 current folder 기준으로만 materialize
- preview decode 비동기
- meta JSON 파싱 캐시 고려

---

## 22. 로깅 및 디버그 정책

### 22.1 로그 레벨
- info
- warning
- error
- debug

### 22.2 주요 로그 포인트
- archive open/close
- create start/end
- extract start/end
- verify start/end
- invalid path 발견
- overwrite 처리 결과
- decode 실패

### 22.3 crash 대응
- 작업 중 임시 상태 로그 남기기
- 실패한 파일 목록 보고서 생성

---

## 23. 에러 처리 정책

사용자에게는 이해 가능한 문장으로 보여주고, 내부 로그에는 원인 세부를 남긴다.

예:
- `Cannot open archive.`
- `Invalid LMDB archive structure.`
- `Entry already exists: Sub/File.jpg`
- `Failed to decode preview image.`
- `Path collision detected on Windows-compatible extraction.`

---

## 24. 보안/안전 정책

- 추출 시 path traversal 방지 (`..` 금지)
- absolute path 생성 금지
- symlink는 MVP에서 미지원 또는 일반 파일로 무시
- huge file / huge key 방어
- 외부 입력 path는 모두 정규화

---

## 25. 구현 단계 제안

### Phase 1: Core
- Path normalization
- LMDB wrapper
- metadata codec
- archive create/open/extract/list
- CLI 기반 검증 가능 수준

### Phase 2: GUI MVP
- MainWindow
- tree/list model
- open/create/extract/delete
- preview pane
- progress dialog

### Phase 3: Verify / Stats / Polish
- verify
- statistics dialog
- error message polishing
- drag & drop
- settings persistence

### Phase 4: Advanced UX
- search
- thumbnail cache
- better sorting/filtering
- rename/replace

---

## 26. Claude Code에 대한 구체 지시사항

### 26.1 구현 스타일
- Qt6 Widgets 사용
- CMake 프로젝트 구성
- C++20 사용
- 플랫폼 종속 코드 최소화
- UI thread block 금지
- core logic과 UI를 명확히 분리
- test 가능한 구조 유지

### 26.2 코드 품질 요구
- RAII 철저 적용
- raw pointer 지양
- 예외 사용 여부 일관성 유지
- 모든 public API에 명확한 책임 부여
- UTF-8 경로 처리 신경 쓸 것
- 대량 데이터에서 불필요한 full copy 줄일 것

### 26.3 우선 구현 순서
1. CLI로 create/list/extract/verify 완성
2. core library 안정화
3. GUI 연결
4. preview 추가
5. 삭제/덮어쓰기/속성
6. polishing

---

## 27. 권장 클래스/메서드 초안

### `class LmdbArchive`
- `bool create(const QString& archivePath, const CreateOptions& options)`
- `bool open(const QString& archivePath, OpenMode mode)`
- `void close()`
- `QVector<ArchiveEntry> listEntries(const QString& prefix)`
- `bool addFile(const QString& sourceFile, const QString& archivePath)`
- `bool addFolder(const QString& sourceFolder, const QString& destPrefix)`
- `QByteArray readFile(const QString& archivePath)`
- `bool extractAll(const QString& outputFolder)`
- `bool extractSelection(const QStringList& paths, const QString& outputFolder)`
- `bool removePath(const QString& pathOrPrefix)`
- `VerifyReport verify() const`
- `ArchiveStats stats() const`

### `class PathUtil`
- `QString normalizeArchivePath(const QString& path)`
- `QStringList splitArchivePath(const QString& path)`
- `QString joinArchivePath(const QStringList& parts)`
- `bool isValidArchivePath(const QString& path)`

### `class ArchiveTreeBuilder`
- `std::unique_ptr<ArchiveTreeNode> build(const QVector<ArchiveEntry>& entries)`

---

## 28. 미결정 이슈

구현 전에 반드시 확정할 것:
1. 아카이브 외형을 디렉토리(`*.lmdb/`)로 할지, 다른 래퍼 구조로 할지
2. 빈 폴더를 지원할지 여부
3. hash 알고리즘을 CRC32 / xxHash64 중 무엇으로 할지
4. overwrite 정책 기본값
5. 대용량 import 시 batch size
6. preview pane 기본 표시 여부
7. 단일 실행 파일에 CLI와 GUI를 통합할지 분리할지

---

## 29. 최종 권장 결정

MVP 기준 권장안:
- 포맷: `*.lmdb/` 디렉토리
- 내부 저장: `entries`, `meta`, `manifest` DBI
- 경로 규칙: `/` 구분자, 상대경로만 허용
- UI: Qt6 Widgets 기반 7-Zip 스타일
- 우선순위: CLI 먼저, 그 후 GUI
- 빈 폴더: 미지원
- 메타데이터: JSON
- 이미지 preview: Qt 기본 디코더 사용
- verify: 필수 제공

---

## 30. 완료 기준 (Definition of Done)

다음이 모두 만족되면 1차 완료로 본다.

1. Windows / Linux에서 빌드 성공
2. 폴더를 아카이브로 생성 가능
3. 아카이브를 열고 트리/리스트 탐색 가능
4. 선택/전체 추출 가능
5. 이미지 preview 가능
6. add/delete 가능
7. verify 결과 리포트 제공
8. CLI create/list/extract/verify 동작
9. round-trip 테스트 통과
10. 한글 경로 테스트 통과

---

## 31. Claude Code용 최종 작업 지시 요약

다음 순서로 작업하라.

1. CMake + Qt6 + LMDB 기반 프로젝트 뼈대를 만든다.
2. core 모듈에서 path normalization, metadata codec, LMDB wrapper를 구현한다.
3. CLI에서 create/list/extract/verify가 완전히 동작하게 만든다.
4. 그 위에 Qt6 Widgets 기반 7-Zip 스타일 GUI를 얹는다.
5. tree/list 탐색, preview, extract, add, delete를 연결한다.
6. verify/statistics/progress/error handling을 정리한다.
7. 테스트를 추가하고 Windows/Linux 모두에서 동작을 검증한다.

이 프로젝트의 핵심은 **LMDB를 단순한 key-value 저장소가 아니라, 사용자가 ZIP/TAR 아카이브처럼 느끼는 제품으로 만드는 것**이다. 저장 구조는 경로 문자열 key를 이용해 가상 폴더를 표현하고, 유지보수성과 디버깅 편의성을 위해 CLI + GUI + verify 기능을 반드시 함께 제공해야 한다.
