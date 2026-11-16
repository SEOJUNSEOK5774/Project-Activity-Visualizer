# 같은 네트워크의 팀원과 웹 대시보드 공유하기

대시보드를 실행할 Windows PC에 팀 DB를 두고, 팀원들은 각자 브라우저에서 HTTPS로 접속합니다. 이 안내에서는 웹 서버 앞에 Caddy를 두고, 프로젝트를 볼 수 있는 계정을 한 명씩 등록합니다. 여러 프로젝트를 제공하려면 프로젝트마다 별도의 팀 DB와 HTTPS 구성을 사용합니다.

## 서버 PC 준비

1. [공식 Caddy 릴리스](https://github.com/caddyserver/caddy/releases/latest)의 **Assets**에서 Windows용 ZIP을 받습니다. 일반적인 Intel·AMD 64비트 Windows에서는 파일명이 `windows_amd64.zip`으로 끝나는 항목입니다. 압축을 풀고 `caddy.exe`를 이 안내 파일과 같은 폴더에 둡니다.
2. 서버 PC의 PowerShell에서 `ipconfig`를 실행합니다. 팀원이 연결된 Wi-Fi나 이더넷의 IPv4 주소를 확인합니다. 아래 명령의 `192.168.1.20`을 실제 주소로 바꿉니다.
3. 서버 PC에서 팀원별 비밀번호 해시를 만듭니다. 패키지 폴더의 PowerShell에서 아래 명령을 팀원마다 실행하면 비밀번호 입력과 확인 입력이 나옵니다. 출력된 해시를 다음 항목에서 사용합니다. [Caddy 비밀번호 명령](https://caddyserver.com/docs/command-line#caddy-hash-password)

   ```powershell
   .\caddy.exe hash-password --algorithm bcrypt
   ```

4. `team-users.example.caddy`를 같은 폴더에 `team-users.caddy`라는 이름으로 복사합니다. 메모장으로 열어 **사용자 이름, 공백, 해당 사용자의 해시** 순으로 한 줄씩 추가합니다. 사용자 이름에는 영문, 숫자, 밑줄, 점, 하이픈을 사용할 수 있습니다. 팀원에게는 자기 계정과 비밀번호를 개별적으로 전달합니다. 계정을 추가하거나 회수하면 파일을 수정하고 HTTPS 실행 창을 재시작합니다. [Caddy 계정 설정](https://caddyserver.com/docs/caddyfile/directives/basic_auth)

## 서버 PC에서 실행

1. 팀 서버가 이미 `http://127.0.0.1:8767/`에서 실행 중이면 2번으로 이동합니다. 팀 서버를 새로 켜려면 `WebDashboard` 폴더의 첫 번째 PowerShell에서 아래 명령을 실행합니다. `--team-db`에 기존 팀 DB의 전체 경로를 지정하면 기존 기록을 이어서 볼 수 있습니다. 공유 폴더와 프로젝트 이름도 팀 설정에 맞춥니다.

   ```powershell
   py -3 -B .\activity_api.py --team-folder "E:\ActivityTeamExchange" --project "AVPackageTest" --team-db "E:\MyTeamData\AVPackageTest-team.db" --port 8767
   ```

2. 두 번째 PowerShell을 패키지 폴더에서 열고 HTTPS를 시작합니다. 두 실행 창을 열어 둡니다.

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Start-TeamHttps.ps1 -ServerIp 192.168.1.20
   ```

3. 팀원 PC에서 연결이 막힐 때는 서버 PC의 관리자 PowerShell에서 다음 명령으로 사설 네트워크의 로컬 서브넷에만 TCP 8443을 허용합니다. 서버 PC의 네트워크 프로필이 **개인(Private)**이어야 이 규칙이 적용됩니다. [Windows 방화벽 규칙](https://learn.microsoft.com/powershell/module/netsecurity/new-netfirewallrule)

   ```powershell
   New-NetFirewallRule -DisplayName "Project Activity HTTPS" -Direction Inbound -Action Allow -Protocol TCP -LocalPort 8443 -Profile Private -RemoteAddress LocalSubnet
   ```

## 각 PC에서 처음 접속

1. HTTPS 서버가 시작되면 서버 PC의 `caddy-data\pki\authorities\local\root.crt`를 찾습니다. 공개 인증서인 이 파일만 접속할 PC에 전달합니다. `caddy-data` 폴더는 서버 PC에 보관합니다. [Caddy 내부 인증서 안내](https://caddyserver.com/docs/automatic-https#local-https)
2. 접속할 Windows PC에서 받은 인증서를 신뢰할 수 있는 파일인지 서버 운영자와 확인합니다. 인증서를 저장한 폴더에서 다음 명령을 실행해 현재 사용자의 신뢰 저장소에 추가합니다. 서버 PC의 브라우저에서 처음 접속할 때도 같은 절차를 사용합니다.

   ```powershell
   certutil.exe -user -addstore Root .\root.crt
   ```

3. 브라우저에서 서버 PC의 HTTPS 주소를 엽니다. 예시 주소는 `https://192.168.1.20:8443/`입니다. 올바른 계정으로 로그인하면 **Team project · AVPackageTest**가 표시됩니다.
4. 다른 PC에서도 같은 에셋의 저장 횟수가 보이는지 확인합니다. 팀원이 언리얼에서 에셋을 저장하고 **Export to team folder**를 누르면 잠시 뒤 두 화면의 수치가 함께 갱신됩니다.

브라우저에 로그인하기 전에 인증이 걸렸는지 확인하려면 인증서를 등록한 Windows PC에서 다음 명령을 실행합니다. 첫 번째는 `401`이 나오고 두 번째는 등록된 사용자 이름과 올바른 비밀번호로 실행했을 때 `200`이 나옵니다. 두 번째 명령은 비밀번호를 묻습니다.

```powershell
curl.exe -sS -o NUL -w "%{http_code}\n" "https://192.168.1.20:8443/api/activity/dashboard"
curl.exe -sS -o NUL -w "%{http_code}\n" -u "alice" "https://192.168.1.20:8443/api/activity/dashboard"
```

## 접속이 안 될 때

- HTTPS 주소에 연결할 수 없으면 두 실행 창이 열려 있는지, 서버 IP가 바뀌었는지, 두 PC가 같은 네트워크에 있는지 확인합니다. 게스트 Wi-Fi는 같은 네트워크 안에서도 기기 간 접속을 제한할 수 있습니다.
- 인증서 오류가 나면 HTTPS 주소의 IP와 `-ServerIp` 값이 같은지, 현재 Caddy가 만든 `root.crt`를 등록했는지 확인합니다.
- 로그인 창이 계속 나오면 사용자 이름과 비밀번호를 확인합니다. 계정 파일을 수정했다면 HTTPS 실행 창을 재시작합니다.
- 로그인 후 `502`가 보이면 팀 서버 창의 주소가 `http://127.0.0.1:8767/`인지 확인합니다.

이 구성의 접속 범위는 같은 사설 네트워크입니다. 팀원에게는 HTTPS 주소를 안내합니다. 팀원이 언리얼에서 **Export to team folder**를 누르면 웹 서버가 공유 폴더에서 새 저장 기록을 가져옵니다.
