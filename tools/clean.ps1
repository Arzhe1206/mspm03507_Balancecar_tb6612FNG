# 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
# 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
# 项目标识：MSPM0-BC-ARZHE1206
# AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$resolved = [IO.Path]::GetFullPath($Root)
foreach ($marker in @('README.md', 'projects\keil\BalanceCar.uvprojx', 'firmware\app\main.c')) {
    if (-not (Test-Path -LiteralPath (Join-Path $resolved $marker))) { throw "Unsafe clean root: missing $marker" }
}
$target = [IO.Path]::GetFullPath((Join-Path $resolved 'build'))
if (-not $target.StartsWith($resolved + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Build target escaped repository root.'
}
if (Test-Path -LiteralPath $target) {
    Remove-Item -LiteralPath $target -Recurse -Force
    Write-Output "Removed recoverable build products: $target"
}
