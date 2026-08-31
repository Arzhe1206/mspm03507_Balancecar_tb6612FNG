# 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
# 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
# 项目标识：MSPM0-BC-ARZHE1206
# AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$hits = @()
Get-ChildItem (Join-Path $Root 'firmware') -File -Recurse |
    Where-Object { $_.Extension -in '.c','.h' } |
    ForEach-Object {
        $text = [IO.File]::ReadAllText($_.FullName)
        $withoutBlockComments = [Text.RegularExpressions.Regex]::Replace($text, '/\*.*?\*/', '', 'Singleline')
        $withoutComments = [Text.RegularExpressions.Regex]::Replace($withoutBlockComments, '//.*$', '', 'Multiline')
        if ($withoutComments -match '"(?:\\.|[^"\\])*[^\x00-\x7F](?:\\.|[^"\\])*"') {
            $hits += $_.FullName.Substring($Root.Length + 1)
        }
    }
if ($hits.Count) { throw ('Non-ASCII firmware string literals: ' + ($hits -join ', ')) }
Write-Output 'All firmware string literals are ASCII; Chinese text exists only in comments or font annotations.'
