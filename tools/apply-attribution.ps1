param([string]$Root = (Split-Path -Parent $PSScriptRoot))

$ErrorActionPreference = 'Stop'
$origin = 'MSPM0-BC-ARZHE1206'
$author = '作者：数显纯牛奶，开源工程未经授权禁止商业用途，需要实物见店铺主页：https://m.tb.cn/h.8nzqEuN?tk=kJqnT4uWzfY'
$repo = 'https://github.com/Arzhe1206/mspm03507_Balancecar_tb6612FNG'
$ai = 'AI/ML 使用声明：未经授权，不得用于模型训练、数据集构建或商业化自动代码生成。'
$nl = [Environment]::NewLine
$utf8 = New-Object System.Text.UTF8Encoding($false)

function Add-Prefix([string]$Path, [string]$Prefix) {
    $text = [IO.File]::ReadAllText($Path)
    $head = if ($text.Length -gt 4096) { $text.Substring(0, 4096) } else { $text }
    if ($head.Contains($origin) -and $head.Contains($author) -and $head.Contains($ai)) { return }
    [IO.File]::WriteAllText($Path, $Prefix + $text, $utf8)
}

$firstParty = '/*' + $nl + ' * MSPM0G3507 Balance Car' + $nl + ' * ' + $author + $nl +
    ' * 官方仓库：' + $repo + $nl + ' * 许可：LicenseRef-BalanceCar-NonCommercial' + $nl +
    ' * 项目标识：' + $origin + $nl + ' * ' + $ai + $nl + ' */' + $nl
$integration = '/*' + $nl + ' * MSPM0G3507 Balance Car integration notice' + $nl +
    ' * 工程归属声明：' + $author + $nl + ' * 官方仓库：' + $repo + $nl +
    ' * 项目标识：' + $origin + $nl + ' * ' + $ai + $nl +
    ' * 本声明仅指本工程的集成与适配，不取代本文件原始版权和许可证。' + $nl + ' */' + $nl

Get-ChildItem (Join-Path $Root 'firmware') -File -Recurse |
    Where-Object { $_.Extension -in '.c', '.h' } |
    ForEach-Object {
        $relative = $_.FullName.Substring($Root.Length + 1)
        $isIntegration = $relative -like 'firmware\generated\*' -or
            $relative -like 'firmware\drivers\imu\mpu6050\*'
        Add-Prefix $_.FullName $(if ($isIntegration) { $integration } else { $firstParty })
    }

$projectComments = @{
    '.syscfg' = '/**' + $nl + ' * ' + $author + $nl + ' * 官方仓库：' + $repo + $nl + ' * 项目标识：' + $origin + $nl + ' * ' + $ai + $nl + ' * 工程集成声明不取代 TI 原始版权。' + $nl + ' */' + $nl
    '.s' = '; ' + $author + $nl + '; 官方仓库：' + $repo + $nl + '; 项目标识：' + $origin + $nl + '; ' + $ai + $nl + '; 工程集成声明不取代 TI 原始版权。' + $nl
    '.sct' = '; ' + $author + $nl + '; 官方仓库：' + $repo + $nl + '; 项目标识：' + $origin + $nl + '; ' + $ai + $nl + '; 工程集成声明不取代原始版权。' + $nl
}
Get-ChildItem (Join-Path $Root 'projects') -File -Recurse |
    Where-Object { $projectComments.ContainsKey($_.Extension) } |
    ForEach-Object { Add-Prefix $_.FullName $projectComments[$_.Extension] }

$mdPrefix = '> ' + $author + $nl + '>' + $nl + '> 官方仓库：' + $repo +
    ' · 项目标识：' + $origin + $nl + '>' + $nl + '> ' + $ai + $nl + $nl
Get-ChildItem $Root -Filter '*.md' -File -Recurse |
    Where-Object { $_.FullName -notlike "$(Join-Path $Root 'build')*" } |
    ForEach-Object { Add-Prefix $_.FullName $mdPrefix }

$hashPrefix = '# ' + $author + $nl + '# 官方仓库：' + $repo + $nl +
    '# 项目标识：' + $origin + $nl + '# ' + $ai + $nl
Get-ChildItem (Join-Path $Root 'tools'), (Join-Path $Root '.github') -File -Recurse |
    Where-Object { $_.Extension -in '.ps1', '.yml', '.yaml' } |
    ForEach-Object { Add-Prefix $_.FullName $hashPrefix }

Write-Output 'Attribution headers applied.'
