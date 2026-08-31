# 作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY
# 官方仓库：https://github.com/Arzhe1206/MSPM0G3507-Self-Balancing-Car
# 项目标识：MSPM0-BC-ARZHE1206
# AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。
param([string]$Root = (Split-Path -Parent $PSScriptRoot), [switch]$Child)

$ErrorActionPreference = 'Stop'
if (-not $Child) {
    $hostExe = (Get-Process -Id $PID).Path
    $process = Start-Process -FilePath $hostExe -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',$PSCommandPath,'-Root',$Root,'-Child') -Wait -PassThru -WindowStyle Hidden
    if ($process.ExitCode -ne 0) { throw "Independent attribution reader failed with exit code $($process.ExitCode)." }
    Write-Output 'Independent child process parsed and verified all attribution headers.'
    exit 0
}
$origin = 'MSPM0-BC-ARZHE1206'
$author = '作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY'
$files = @()
$files += Get-ChildItem (Join-Path $Root 'firmware') -File -Recurse | Where-Object { $_.Extension -in '.c','.h' }
$files += Get-ChildItem $Root -Filter '*.md' -File -Recurse | Where-Object { $_.FullName -notlike "$(Join-Path $Root 'build')*" }
$files += Get-ChildItem (Join-Path $Root 'projects') -File -Recurse | Where-Object { $_.Extension -in '.syscfg','.s','.sct' }
$files += Get-ChildItem (Join-Path $Root 'tools'),(Join-Path $Root '.github') -File -Recurse | Where-Object { $_.Extension -in '.ps1','.yml','.yaml' }
$failures = @()
foreach ($file in $files | Sort-Object FullName -Unique) {
    $head = (Get-Content -LiteralPath $file.FullName -TotalCount 40) -join [Environment]::NewLine
    if (-not $head.Contains($origin) -or -not $head.Contains($author)) {
        $failures += $file.FullName.Substring($Root.Length + 1)
    }
}
if ($failures.Count) { throw ('Missing or displaced attribution: ' + ($failures -join ', ')) }
exit 0
