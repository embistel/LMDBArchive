# LMDB Archive 구현 계획서 (단일 `.lmdb` 파일 중심)

## 1. 목표

`LMDB Archive`는 **LMDB 기반 단일 파일 아카이브(`.lmdb`)를 7-Zip처럼 탐색, 생성, 추출, 검증, 갱신**할 수 있는 크로스플랫폼 데스크톱 유틸리티이다.

이 문서는 Claude Code가 **처음부터 완성도 높은 구현**을 할 수 있도록, 요구사항을 설계 수준까지 구체화한 구현 계획서다.

이번 개정판의 핵심 목표는 다음과 같다.

- 사용자가 다루는 결과물은 **일반적인 LMDB 디렉토리(`data.mdb`, `lock.mdb`)가 아니라 ZIP처럼 보이는 단일 `.lmdb` 파일**이어야 한다.
- 구현은 가능한 한 **기존 LMDB C 라이브러리**를 그대로 활용한다.
- **임시 폴더에 풀었다가 다시 읽는 방식은 금지**한다.
- **`.lmdb` 파일 자체를 직접 open / mmap** 하여 운용한다.
- 프로그램 전체의 기본 운용 방식은 **단일 `.lmdb` 파일 중심**이어야 하며, 디렉토리형 LMDB는 기본 UX에서 배제한다.

---

## 2. 핵심 설계 결론

### 2.1 최종 저장 형식
사용자에게 노출되는 결과물은 **하나의 `.lmdb` 파일**이다.

예:
- `defects.lmdb`
- `review_2026_04_23.lmdb`
- `archive_001.lmdb`

이 파일은 내부적으로 **LMDB environment의 main data file 자체**로 사용한다.

### 2.2 왜 이렇게 가는가
기존 LMDB는 기본적으로 디렉토리 아래 `data.mdb`와 `lock.mdb` 두 파일을 사용한다. 하지만 `MDB_NOSUBDIR` 옵션을 사용하면, 지정한 path 자체를 **데이터 파일**로 사용한다. 따라서 표준 LMDB만으로도 **단일 data file 운용은 가능**하다. 다만 lock 정책은 별개이며, 기본 lock을 쓰면 sidecar lock 파일이 생길 수 있고, 완전한 단일 파일 운용을 원하면 `MDB_NOLOCK`와 애플리케이션 레벨 동기화를 함께 설계해야 한다. citeturn217507search3turn217507search8turn217507search19

따라서 본 프로젝트는 다음 전략을 채택한다.

- **기본 아카이브 파일 = 단일 `.lmdb` data file**
- 내부 접근은 **LMDB 표준 라이브러리 사용**
- **임시 추출/재패킹 금지**
- 기본 운용 정책은 **`MDB_NOSUBDIR` 기반 단일 파일 open**
- 기본 제품 UX는 **“파일 하나를 복사하면 이동된다”**에 맞춘다

### 2.3 하지 않을 것
다음 방식은 **채택하지 않는다**.

1. `.lmdb` 컨테이너 파일 안에 `data.mdb` / `lock.mdb`를 다시 집어넣고, 사용할 때 임시 폴더에 풀어서 open
   - 큰 파일 복사/재생성 비용이 큼
   - mmap 장점 상실
   - 단일 파일처럼 보여도 실제 운용 효율이 나쁨

2. `.lmdb` 큰 파일 내부의 특정 오프셋에 embedded LMDB를 넣고, 기존 LMDB 라이브러리가 그 내부 영역을 직접 mmap 하도록 사용
   - 표준 LMDB API는 custom I/O backend나 custom VFS hook을 제공하지 않음
   - `mdb_env_open()`은 path를 받아 실제 파일을 mmap 하는 모델이므로, **컨테이너 내부 서브파일을 직접 여는 방식은 현실적이지 않음**. citeturn217507search3turn217507search9

즉, **가장 현실적이고 빠른 해법은 `.lmdb` 파일 자체를 LMDB의 main data file로 쓰는 것**이다.

---

## 3. 제품 목표

### 3.1 제품 목표
- **동작과 UI는 7-Zip과 동일한 감각**을 제공한다.
- **Windows / Linux 동시 지원**을 위해 **Qt6** 기반으로 구현한다.
- 폴더를 `.lmdb` 아카이브로 만들거나, `.lmdb` 아카이브를 폴더로 풀 수 있어야 한다.
- 저장 규칙은 실제 폴더 구조를 key 경로 문자열로 보존한다.
  - 루트 파일: `File.jpg`
  - 하위 폴더 파일: `SubFolder/File.jpg`
  - 다중 하위 폴더: `Sub1/Sub2/File.jpg`
- 사용자는 결과물을 **ZIP처럼 “하나의 파일”**로 인식해야 한다.
- 외부 라이브러리나 타 프로그램에서 접근할 때도 기본적으로 **`.lmdb` 파일 하나만 열어서 읽고 쓸 수 있어야 한다.**

### 3.2 사용자 관점 기능
1. `.lmdb` 아카이브 열기
2. `.lmdb` 아카이브 내부를 폴더 트리처럼 탐색
3. 파일 미리보기(특히 jpg/png/bmp 등 이미지)
4. 폴더 → `.lmdb` 아카이브 만들기
5. `.lmdb` 아카이브 → 폴더로 추출하기
6. 선택 항목만 추출하기
7. 아카이브 내부 파일 추가 / 삭제 / 덮어쓰기
8. 무결성 검사(verify)
9. 기본 통계 보기(파일 수, 총 바이트 수, 확장자 분포 등)
10. drag & drop 지원
11. 명령행 모드 제공

---

## 4. 제품 범위

### 4.1 1차 버전(MVP)
반드시 구현:
- 단일 `.lmdb` 파일 생성
- 단일 `.lmdb` 파일 열기
- 트리/리스트 기반 탐색 UI
- 전체 추출 / 선택 추출
- 파일 추가 / 폴더 추가
- 파일 삭제
- 기본 이미지 미리보기
- Verify
- CLI 기본 명령
- 단일 파일 기준 잠금 / 단일 writer 정책
- 외부 라이브러리 접근 규칙 문서화

### 4.2 2차 버전
- 파일 교체
- 대량 작업 진행률 상세화
- 썸네일 캐시
- 검색 / 필터 / 정렬 강화
- 아카이브 비교(diff)
- read-only / repair 모드
- Compatibility Mode에서 sidecar lock 파일 허용 여부 선택 UI

### 4.3 3차 버전
- 가상 파일시스템 마운트 연동 검토
- 내부 메타데이터 버전 업그레이드 툴
- 별도 lock broker 프로세스 검토

---

## 5. UX 원칙

### 5.1 7-Zip과 동일한 감각
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

### 5.2 성능 우선 UX
- 대량 파일 목록 표시 시 lazy loading 또는 batched model refresh 고려
- 큰 이미지 미리보기는 원본 전체 decode 대신 제한 해상도 preview 사용
- 진행률 표시와 cancel 지원
- UI thread block 금지
- **아카이브 open 시 임시 추출 금지**
- **추출 없이 직접 browse / preview / export**
- 기본 UX 상 사용자는 항상 **파일 하나**를 열고 닫는다고 느껴야 한다

---

## 6. 저장 모델 설계

### 6.1 기본 개념
LMDB는 실제 폴더 구조를 갖지 않으므로, **key를 경로 문자열(path-like key)** 로 저장한다.

예시:
- `File.jpg`
- `SubFolder/File.jpg`
- `SubFolder/Nested/File.jpg`

value는 파일의 raw bytes이다.

즉:
- key = 논리적 파일 경로
- value = 파일 내용(binary)

### 6.2 디렉토리 표현 방식
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

### 6.3 빈 폴더 처리 정책
기본 정책:
- **MVP에서는 빈 폴더를 지원하지 않는다.**
- 폴더는 하위 파일이 하나 이상 있을 때만 존재한다고 본다.

확장 정책(옵션):
- 예약된 special key로 빈 폴더 메타를 저장할 수 있음
- 예: `__dir__/EmptyFolder`
- 단, MVP에서는 미구현

---

## 7. 단일 `.lmdb` 파일 설계

### 7.1 파일 포맷 관점의 정의
사용자가 보는 `.lmdb` 파일은 사실상 **LMDB main data file**이다.

즉:
- `.lmdb` = LMDB의 `data.mdb` 역할
- 별도 디렉토리 없음
- 별도 `data.mdb` 파일명 노출 없음
- 기본 UX에서 관리 대상은 항상 `.lmdb` 파일 하나

### 7.2 LMDB open 정책
아카이브 open 시:
- `mdb_env_create()`
- `mdb_env_set_maxdbs()`
- `mdb_env_set_mapsize()`
- `mdb_env_open(path_to_file, MDB_NOSUBDIR | [필요 시 MDB_NOLOCK] | 기타 옵션, mode)`

`MDB_NOSUBDIR` 사용 시 path는 디렉토리가 아니라 **main data file path**로 해석된다. 기본 lock을 쓰면 같은 이름에 `-lock`이 붙는 보조 파일이 생길 수 있다. `MDB_NOLOCK` 사용 시 LMDB의 lock file을 사용하지 않으며, 동시성 관리를 호출자가 직접 책임져야 한다. citeturn217507search3turn217507search8turn217507search19

### 7.3 기본 운용 모드
본 프로젝트는 다음 두 모드를 정의한다.

#### A. Single File Archive Mode (기본)
- `.lmdb` 단일 파일만 사용자에게 보여준다.
- LMDB는 `MDB_NOSUBDIR | MDB_NOLOCK`로 open 한다.
- 잠금은 애플리케이션 레벨에서 직접 관리한다.
- 단일 writer 보장.
- UI와 CLI 모두 기본적으로 이 모드로 동작한다.
- 이 모드의 목적은 **“ZIP처럼 파일 하나만 복사해서 이동 가능”**을 만족하는 것이다.

#### B. Compatibility Mode (옵션)
- `MDB_NOSUBDIR`만 사용하고, LMDB 기본 `-lock` 파일 생성을 허용한다.
- 외부 LMDB 도구와의 호환성 극대화.
- 단, 사용자 관점에서는 sidecar lock file이 생길 수 있다.
- 기본 제품 UX가 아니라 고급 옵션으로만 제공한다.

MVP의 기본값은 **A 모드**로 한다.

### 7.4 외부 라이브러리 접근 규칙
이 프로그램이 생성한 `.lmdb` 파일은 **기본적으로 표준 LMDB 단일 data file**이다. 따라서 다른 라이브러리나 타 프로그램에서 접근할 때도 다음 규칙을 따르면 포맷 차원에서는 호환된다.

- 파일을 열 때 **`MDB_NOSUBDIR`로 open** 해야 한다.
- 기본 프로그램 운용과 충돌하지 않으려면, 외부 접근도 **Single File Archive Mode 정책** 또는 **Compatibility Mode 정책** 중 하나를 명확히 따라야 한다.
- **동시성 문제가 없다면**, 외부 프로그램이 같은 `.lmdb` 파일을 `MDB_NOSUBDIR`로 읽고 쓰는 데 큰 문제는 없다.
- 다만 이는 **포맷 차원의 호환성**이고, **동시성 안전성은 lock 정책과 접근 규율에 달려 있다.**

즉 문서에 반드시 다음 문장을 명시한다.

> 이 프로젝트의 `.lmdb` 파일은 기본적으로 `MDB_NOSUBDIR` 기반 단일 data file 이며, 외부 라이브러리도 `MDB_NOSUBDIR`로 열면 포맷상 호환된다. 단, 동시성 안전성은 별도 문제이며, `MDB_NOLOCK` 기반 단일 파일 운용에서는 애플리케이션 레벨의 외부 동기화가 필요하다.

### 7.5 왜 기본을 `MDB_NOLOCK`로 잡는가
사용자 요구사항은 다음 세 가지를 동시에 만족해야 한다.

- 결과물이 **단일 `.lmdb` 파일**이어야 함
- **임시 추출 금지**
- **파일 자체를 직접 access**해야 함

이 세 가지를 표준 LMDB로 동시에 만족시키려면 사실상 **`MDB_NOSUBDIR + MDB_NOLOCK` + 자체 잠금 정책**이 가장 현실적이다. LMDB 공식 문서상 `MDB_NOLOCK`에서는 호출자가 single-writer semantics와 오래된 read transaction 부재를 직접 보장해야 한다. citeturn217507search19turn217507search12

### 7.6 애플리케이션 레벨 잠금 정책
다음 정책을 구현한다.

1. **한 번에 하나의 writer만 허용**
2. read-only open은 허용하되, write 중일 때 다른 process의 read-only open 정책은 초기 버전에서 보수적으로 제한
3. `.lmdb` 파일 자체에 대해 별도 lock primitive 사용
   - Windows: 파일 핸들 기반 잠금
   - Linux: `flock` 또는 `fcntl` 기반 잠금
4. 같은 애플리케이션 내부에서는 `QLockFile` 또는 OS native file lock wrapper로 상위 제어
5. write 세션 중 abnormal exit 시, 다음 open에서 recovery/verify 수행
6. 프로그램 전체 설계는 **“항상 단일 `.lmdb` 파일을 기준으로 lock”** 해야 한다. 별도 디렉토리 lock 모델을 가정하지 않는다.

중요: `MDB_NOLOCK`을 쓰면 LMDB의 reader table도 사용하지 않으므로, **동시 read/write 허용 범위는 반드시 제품 정책으로 좁게 잡아야 한다**. citeturn217507search1turn217507search19

### 7.7 파일 이동성 규칙
기본 모드에서 `.lmdb` 파일은 **파일 하나만 복사하면 이동 가능**해야 한다.

즉:
- 파일 백업 = `.lmdb` 파일 단일 복사
- 파일 전달 = `.lmdb` 파일 단일 복사
- 파일 아카이빙 = `.lmdb` 파일 단일 보관

Compatibility Mode에서 sidecar `-lock` 파일이 잠깐 생길 수 있지만, 그것은 런타임 동시성 파일일 뿐이며 기본 배포/보관 단위로 간주하지 않는다.

---

## 8. 주요 유스케이스 흐름

### 8.1 폴더 → 단일 `.lmdb` 아카이브 생성
동작:
1. 대상 폴더 재귀 순회
2. 상대 경로 산출
3. `/` 기준의 canonical key 생성
4. `.lmdb` 파일을 `MDB_NOSUBDIR | MDB_NOLOCK`로 open
5. write transaction에서 파일 바이트 저장
6. chunk 단위 commit
7. verify 수행
8. 최종 결과물은 **`.lmdb` 파일 하나**

### 8.2 단일 `.lmdb` 아카이브 → 폴더 추출
동작:
1. `.lmdb` 파일 open
2. key 전체 scan 또는 prefix scan
3. 각 key에 대해 대상 경로 생성
4. 상위 디렉토리 생성
5. bytes 기록
6. modified time 복원(optional)
7. 검증(optional)

### 8.3 아카이브 열기
동작:
1. `.lmdb` 파일을 read-only 또는 read-write로 open
2. manifest 확인
3. index build(메모리 트리 생성)
4. UI 트리/리스트 갱신

---

## 9. UX 세부 동작 정의

### 9.1 메인 윈도우 레이아웃
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
- Statistics
- Options
- Reopen in Compatibility Mode

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

## 13. Verify / Recovery 정책

### 13.1 Verify
검사 항목:
- manifest 존재 여부
- required DBI 존재 여부
- 각 `entries` key에 대응하는 `meta` 존재 여부
- file_size 일치 여부
- hash 일치 여부(optional)
- path normalization 위반 여부
- duplicate / invalid key 여부
- 단일 파일 모드 정책 위반 여부

### 13.2 Recovery
MVP에서는 자동 repair를 적극적으로 하지 말고, **진단 리포트 생성** 수준으로 시작한다.

예:
- orphan entry
- missing meta
- invalid path
- corrupted meta JSON
- 비정상 종료 후 재open 시 상태 점검

2차 버전에서 선택적 repair 지원 검토.

---

## 14. CLI 설계

GUI만 있으면 자동화와 디버그에 약하므로 CLI를 같이 제공한다.

실행 파일 예:
- `lmdb-archive`

### 14.1 기본 명령
- `create <folder> <archive.lmdb>`
- `extract <archive.lmdb> <folder>`
- `list <archive.lmdb> [prefix]`
- `verify <archive.lmdb>`
- `stat <archive.lmdb>`
- `add <archive.lmdb> <file_or_folder> [dest_prefix]`
- `delete <archive.lmdb> <path_or_prefix>`
- `cat <archive.lmdb> <internal_path> <output_file>`
- `compat-open <archive.lmdb>`

### 14.2 예시
```bash
lmdb-archive create ./input ./defects.lmdb
lmdb-archive list ./defects.lmdb SubFolder/
lmdb-archive extract ./defects.lmdb ./out
lmdb-archive verify ./defects.lmdb
lmdb-archive cat ./defects.lmdb SubFolder/File.jpg ./File.jpg
```

---

## 15. 라이브러리 접근 가이드 통합 요구사항

Claude Code는 구현과 함께 다음 내용을 코드와 문서에 반영해야 한다.

1. `.lmdb` 파일은 외부 C/C++ 코드에서 **표준 LMDB API로 접근 가능**해야 한다.
2. 외부 샘플 코드에는 반드시 **`MDB_NOSUBDIR` 사용 예시**가 포함되어야 한다.
3. 문서에는 다음을 분리해서 설명해야 한다.
   - 포맷 호환성
   - lock 정책
   - 단일 파일 이동성
   - 동시성 제한
4. 기본 제품 UX는 **단일 파일 모드**를 기준으로 문서화해야 하며, Compatibility Mode는 부가 설명으로만 둔다.

---

## 16. 프로젝트 구조 제안

```text
LMDBArchive/
  CMakeLists.txt
  README.md
  docs/
    lmdb_archive_implementation_plan.md
    lmdb_single_file_library_guidelines.md
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
      ArchiveLock.h/.cpp
    lmdb/
      LmdbEnv.h/.cpp
      LmdbArchive.h/.cpp
      LmdbCursor.h/.cpp
      ArchiveEnvironment.h/.cpp
      ArchiveStore.h/.cpp
      ArchiveTransaction.h/.cpp
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
    test_single_file_mode.cpp
```

---

## 17. 구현 시 최우선 확인 사항

Claude Code는 다음 우선순위를 지켜야 한다.

1. **기본 UX는 항상 단일 `.lmdb` 파일 중심이어야 한다.**
2. **임시 디렉토리 추출 방식은 사용하지 않는다.**
3. **모든 기본 open/create 흐름은 `MDB_NOSUBDIR` 기준으로 설계한다.**
4. **기본 제품 모드는 sidecar 없는 Single File Archive Mode 이다.**
5. **외부 라이브러리 접근 시에도 `MDB_NOSUBDIR`로 열면 호환된다는 점을 문서와 샘플 코드에 반영한다.**
6. **동시성은 포맷 문제가 아니라 lock 정책 문제임을 문서에 명확히 적는다.**

