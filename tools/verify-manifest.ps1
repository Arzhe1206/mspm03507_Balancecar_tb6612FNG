# 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
# 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
# 项目标识：MSPM0-BC-ARZHE1206
# AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$manifest = Join-Path $Root 'release\MANIFEST.sha256'
foreach ($line in Get-Content -LiteralPath $manifest) {
    if ($line -notmatch '^([0-9A-Fa-f]{64}) \*(.+)$') { throw "Invalid manifest line: $line" }
    $path = Join-Path $Root $Matches[2]
    if (-not (Test-Path -LiteralPath $path)) { throw "Manifest file missing: $($Matches[2])" }
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $Matches[1]) { throw "Hash mismatch: $($Matches[2])" }
}
Write-Output 'Release manifest verified.'
