# 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
# 官方仓库：https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG
# 项目标识：MSPM0-BC-ARZHE1206
# AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$required = @('MSPM0-BC-ARZHE1206','https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG','https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY')
foreach ($relative in @('README.md','NOTICE.md','LICENSE','firmware\config\project_identity.h')) {
    $text = [IO.File]::ReadAllText((Join-Path $Root $relative))
    foreach ($token in $required) {
        if (-not $text.Contains($token)) { throw "Missing origin token in ${relative}: $token" }
    }
}
Write-Output 'Project origin identity is internally consistent.'
