[xml]$xml = Get-Content CS2ESP_Driver.vcxproj
$ns = @{ msbuild = 'http://schemas.microsoft.com/developer/msbuild/2003' }

# Check both Release x64 and Debug x64 configurations
$configs = $xml.SelectNodes("//msbuild:ItemDefinitionGroup[@Condition=""'`$(Configuration)|`$(Platform)'=='Release|x64'""]", $ns)
$configs += $xml.SelectNodes("//msbuild:ItemDefinitionGroup[@Condition=""'`$(Configuration)|`$(Platform)'=='Debug|x64'""]", $ns)

foreach ($config in $configs) {
	# Check if Link already exists
	$linkNode = $config.SelectSingleNode("msbuild:Link", $ns)
	if ($null -eq $linkNode) {
		# Create Link element
		$linkNode = $xml.CreateElement("Link", "http://schemas.microsoft.com/developer/msbuild/2003")

		$subSystem = $xml.CreateElement("SubSystem", "http://schemas.microsoft.com/developer/msbuild/2003")
		$subSystem.InnerText = "Native"
		$linkNode.AppendChild($subSystem) | Out-Null

		$driver = $xml.CreateElement("Driver", "http://schemas.microsoft.com/developer/msbuild/2003")
		$driver.InnerText = "Driver"
		$linkNode.AppendChild($driver) | Out-Null

		$deps = $xml.CreateElement("AdditionalDependencies", "http://schemas.microsoft.com/developer/msbuild/2003")
		$deps.InnerText = "ntoskrnl.lib;hal.lib;wmilib.lib;%(AdditionalDependencies)"
		$linkNode.AppendChild($deps) | Out-Null

		# Insert Link before DriverSign
		$driverSignNode = $config.SelectSingleNode("msbuild:DriverSign", $ns)
		if ($null -ne $driverSignNode) {
			$config.InsertBefore($linkNode, $driverSignNode) | Out-Null
		} else {
			$config.AppendChild($linkNode) | Out-Null
		}
	}
}

$xml.Save((Resolve-Path CS2ESP_Driver.vcxproj).Path)
Write-Host "Successfully updated CS2ESP_Driver.vcxproj with Link configuration"
