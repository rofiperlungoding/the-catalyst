$username = "255150307111073"
$password = "Bu4tR4w0n!!"
$cookieFile = "cookies.txt"

Write-Host "--- BRONE VERIFICATION (PowerShell + Curl.exe) ---" -ForegroundColor Cyan

# Cleanup old cookies
if (Test-Path $cookieFile) { Remove-Item $cookieFile }

Write-Host ">> STEP 1: Initial Landing..."
$res1 = curl.exe -s -k -L -c $cookieFile "https://brone.ub.ac.id/login/index.php"

Write-Host ">> STEP 2: Triggering SSO Redirect..."
$res2 = curl.exe -s -k -L -b $cookieFile -c $cookieFile "https://brone.ub.ac.id/auth/saml2/login.php?wants=https%3A%2F%2Fbrone.ub.ac.id%2Fmy%2F"

# Extract form action and SAML hidden fields
if ($res2 -match 'action="([^"]+)"') {
    $formAction = $Matches[1]
    Write-Host "   SSO Form found."
} else {
    Write-Host "   FAILED: Could not find SSO form. Cloudflare might be blocking." -ForegroundColor Red
    exit
}

# Prepare Login Data
# Manual parsing hidden fields from IAM UB (lt, execution, _eventId)
$lt = ""
if ($res2 -match 'name="lt" value="([^"]+)"') { $lt = $Matches[1] }
$execution = ""
if ($res2 -match 'name="execution" value="([^"]+)"') { $execution = $Matches[1] }

$postData = "username=$username&password=[PASSWORD]&lt=$lt&execution=$execution&_eventId=submit&submit=LOGIN"
# Replacing password manually for the curl command to avoid logging it
$postDataFinal = $postData.Replace("[PASSWORD]", [uri]::EscapeDataString($password))

Write-Host ">> STEP 3: Submitting Credentials..."
$res3 = curl.exe -s -k -L -b $cookieFile -c $cookieFile -X POST -d "$postDataFinal" -H "Content-Type: application/x-www-form-urlencoded" "$formAction"

# Check for SAMLResponse (Moodle needs this to finalize session)
if ($res3 -match 'name="SAMLResponse" value="([^"]+)"') {
    $samlResponse = $Matches[1]
    if ($res3 -match 'action="([^"]+)"') {
        $samlAction = $Matches[1]
        Write-Host "   SAML Response received. Finalizing Brone session..."
        $res4 = curl.exe -s -k -L -b $cookieFile -c $cookieFile -X POST -d "SAMLResponse=$([uri]::EscapeDataString($samlResponse))" -H "Content-Type: application/x-www-form-urlencoded" "$samlAction"
    }
}

Write-Host ">> STEP 4: Fetching Dashboard..."
$dashboard = curl.exe -s -k -L -b $cookieFile -c $cookieFile "https://brone.ub.ac.id/my/"

if ($dashboard -match '"sesskey":"([^"]+)"') {
    $sesskey = $Matches[1]
    Write-Host "   SUCCESS! Logged in. Sesskey: $sesskey" -ForegroundColor Green
    
    # FETCH DATA
    Write-Host ">> STEP 5: Fetching Tasks via AJAX..."
    $ajaxUrl = "https://brone.ub.ac.id/lib/ajax/service.php?sesskey=$sesskey&info=core_calendar_get_action_events_by_timesort"
    $payload = '[{"index":0,"methodname":"core_calendar_get_action_events_by_timesort","args":{"limitnum":5,"timesortfrom":' + [Math]::Floor((Get-Date).AddDays(-1).ToFileTimeUtc()/10000000 - 11644473600) + ',"timesortto":' + [Math]::Floor((Get-Date).AddDays(30).ToFileTimeUtc()/10000000 - 11644473600) + '}}]'
    
    $tasksJson = curl.exe -s -k -b $cookieFile -X POST -d $payload -H "Content-Type: application/json" "$ajaxUrl"
    
    Write-Host "`n--- TASKS FROM BRONE ---" -ForegroundColor Yellow
    # Simple search for task names if you don't have a JSON parser ready in PS
    # But we'll just output the raw count for now as verification
    if ($tasksJson -match '"name":"([^"]+)"') {
        Write-Host "Tasks found! Logic is working." -ForegroundColor Green
        # Displaying raw JSON for debug proof
        Write-Host $tasksJson.Substring(0, [Math]::Min(500, $tasksJson.Length)) "..."
    } else {
        Write-Host "No tasks found or failed to parse JSON."
    }
} else {
    Write-Host "   FAILED: Still couldn't get session. You might need to check your password or if SSO has a captcha." -ForegroundColor Red
}

# Cleanup
if (Test-Path $cookieFile) { Remove-Item $cookieFile }
