# 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
# 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
# 项目标识：MSPM0-BC-ARZHE1206
# AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$release = Join-Path $Root 'release'
New-Item -ItemType Directory -Path $release -Force | Out-Null
$output = Join-Path $release 'MANIFEST.sha256'
$rows = Get-ChildItem $Root -File -Recurse |
    Where-Object {
        $_.FullName -notlike "$(Join-Path $Root 'build')*" -and
        $_.FullName -notlike "$(Join-Path $Root '.git')*" -and
        $_.FullName -ne $output
    } |
    Sort-Object FullName |
    ForEach-Object {
        $relative = $_.FullName.Substring($Root.Length + 1).Replace('\','/')
        $hash = Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256 | Select-Object -ExpandProperty Hash
        $hash + ' *' + $relative
    }
[IO.File]::WriteAllLines($output, $rows, (New-Object Text.UTF8Encoding($false)))
Write-Output $output
