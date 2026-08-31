# 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
# 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
# 项目标识：MSPM0-BC-ARZHE1206
# AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$project = Join-Path $Root 'projects\keil\BalanceCar.uvprojx'
[xml]$xml = [IO.File]::ReadAllText($project)
$base = Split-Path -Parent $project
$missing = @()
foreach ($node in $xml.SelectNodes('//FilePath')) {
    $path = [IO.Path]::GetFullPath((Join-Path $base $node.InnerText))
    if (-not (Test-Path -LiteralPath $path)) { $missing += $node.InnerText }
}
$includeNode = $xml.SelectNodes('//IncludePath') | Where-Object { $_.InnerText.Trim() } | Select-Object -First 1
foreach ($entry in $includeNode.InnerText.Split(';')) {
    $path = [IO.Path]::GetFullPath((Join-Path $base $entry))
    if (-not (Test-Path -LiteralPath $path)) { $missing += "include:$entry" }
}
$raw = [IO.File]::ReadAllText($project)
if ($raw -match 'ALL_MSPM0G3507_Project|C:\\Users\\|D:\\Ti\\|\.\.\\source|\.\.\\BSP') {
    throw 'Stale or private Keil path detected.'
}
if ($missing.Count) { throw ('Missing project paths: ' + ($missing -join ', ')) }
Write-Output 'Keil project references are relative and complete.'
