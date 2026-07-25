# Take ownership of Component Categories\{00021493...} key and grant Administrators full control
$keyPath = "SOFTWARE\Classes\Component Categories\{00021493-0000-0000-C000-000000000046}"
try {
    $key = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey($keyPath, [Microsoft.Win32.RegistryKeyPermissionCheck]::ReadWriteSubTree, [System.Security.AccessControl.RegistryRights]::TakeOwnership)
    if ($key) {
        $acl = $key.GetAccessControl()
        $admins = New-Object System.Security.Principal.NTAccount("Administrators")
        $acl.SetOwner($admins)
        $key.SetAccessControl($acl)
        $key.Close()
        Write-Host "Ownership taken"
        $key2 = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey($keyPath, [Microsoft.Win32.RegistryKeyPermissionCheck]::ReadWriteSubTree, [System.Security.AccessControl.RegistryRights]::ChangePermissions)
        $acl2 = $key2.GetAccessControl()
        $rule = New-Object System.Security.AccessControl.RegistryAccessRule($admins, [System.Security.AccessControl.RegistryRights]::FullControl, [System.Security.AccessControl.InheritanceFlags]::ContainerInherit, [System.Security.AccessControl.PropagationFlags]::None, [System.Security.AccessControl.AccessControlType]::Allow)
        $acl2.SetAccessRule($rule)
        $key2.SetAccessControl($acl2)
        $key2.Close()
        Write-Host "Administrators granted FullControl"
    } else {
        Write-Host "Key not found"
    }
} catch {
    Write-Host "Failed: " $_.Exception.Message
}
