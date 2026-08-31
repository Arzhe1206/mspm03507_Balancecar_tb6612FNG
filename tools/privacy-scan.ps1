# 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
# 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
# 项目标识：MSPM0-BC-ARZHE1206
# AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$patterns = 'C:\\Users\\|[A-Za-z]:\\Ti\\|Documents\\Codex|Downloads\\|\.uvguix\.|\.uvoptx'
$extensions = '.c','.h','.md','.ps1','.yml','.yaml','.xml','.uvprojx','.syscfg','.s','.sct','.txt','.cff'
$hits = Get-ChildItem $Root -File -Recurse |
    Where-Object {
        $_.FullName -notlike "$(Join-Path $Root 'build')*" -and
        $_.FullName -notlike "$(Join-Path $Root '.git')*" -and
        $_.FullName -ne $PSCommandPath -and
        $_.FullName -ne (Join-Path $Root 'tools\verify-project.ps1') -and
        $_.Extension -in $extensions
    } |
    Select-String -Pattern $patterns -ErrorAction SilentlyContinue
if ($hits) {
    $hits | ForEach-Object { Write-Error "$($_.Path):$($_.LineNumber): $($_.Line.Trim())" }
    exit 1
}
Write-Output 'No private machine paths or Keil user-state files detected.'
