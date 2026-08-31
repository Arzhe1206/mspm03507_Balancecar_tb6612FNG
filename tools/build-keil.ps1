# 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
# 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
# 项目标识：MSPM0-BC-ARZHE1206
# AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
param([string]$Root = (Split-Path -Parent $PSScriptRoot), [switch]$Rebuild)

$ErrorActionPreference = 'Stop'
$uv4 = 'C:\Keil_v5\UV4\UV4.exe'
$project = Join-Path $Root 'projects\keil\BalanceCar.uvprojx'
$logDir = Join-Path $Root 'build\logs'
$log = Join-Path $logDir 'keil-build.log'
if (-not (Test-Path -LiteralPath $uv4)) { throw "Keil not found: $uv4" }
if (-not (Test-Path -LiteralPath $project)) { throw "Project not found: $project" }
New-Item -ItemType Directory -Path $logDir -Force | Out-Null
$action = if ($Rebuild) { '-r' } else { '-b' }
$process = Start-Process -FilePath $uv4 -ArgumentList @($action, $project, '-j0', '-o', $log) -Wait -PassThru -WindowStyle Hidden
Get-Content -LiteralPath $log
if ($process.ExitCode -ne 0 -or (Select-String -LiteralPath $log -Pattern '[1-9][0-9]* Error\(s\)' -Quiet)) { exit 1 }
