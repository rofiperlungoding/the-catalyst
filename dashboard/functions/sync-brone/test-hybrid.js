import { execSync } from 'child_process';
import * as cheerio from 'cheerio';
import fs from 'fs';

const BRONE_USERNAME = "255150307111073";
const BRONE_PASSWORD = "Bu4tR4w0n!!";
const COOKIE_FILE = 'cookies.txt';

function curl(url, options = {}) {
  let cmd = `curl.exe -s -k -L -b ${COOKIE_FILE} -c ${COOKIE_FILE} `;
  if (options.method === 'POST') {
    cmd += `-X POST -d "${options.body}" -H "Content-Type: application/x-www-form-urlencoded" `;
  }
  cmd += `"${url}"`;
  return execSync(cmd).toString();
}

async function testSync() {
  console.log("--- BRONE SYNC TEST (Hybrid Node+Curl.exe) ---");

  if (fs.existsSync(COOKIE_FILE)) fs.unlinkSync(COOKIE_FILE);

  try {
    // 1. Initial Landing
    console.log(">> STEP 1: Initial Landing...");
    const html1 = curl('https://brone.ub.ac.id/login/index.php');
    if (html1.includes('login')) console.log("   Initial page fetched.");

    // 2. SSO Redirect
    console.log(">> STEP 2: Triggering SSO Redirect...");
    const ssoHtml = curl('https://brone.ub.ac.id/auth/saml2/login.php?wants=https%3A%2F%2Fbrone.ub.ac.id%2Fmy%2F');
    let $ = cheerio.load(ssoHtml);
    const formAction = $('form').attr('action');

    if (!formAction) {
      console.log("   FAILED: SSO form not found. Page content:");
      console.log(ssoHtml.substring(0, 1000));
      return;
    }
    console.log("   SSO Form found:", formAction);

    // Prepare credentials
    const formData = [];
    $('input[type="hidden"]').each((i, el) => {
      formData.push(`${$(el).attr('name')}=${encodeURIComponent($(el).attr('value'))}`);
    });
    formData.push(`username=${BRONE_USERNAME}`);
    formData.push(`password=${encodeURIComponent(BRONE_PASSWORD)}`);

    console.log(">> STEP 3: Submitting Credentials to IAM UB...");
    const loginResHtml = curl(formAction, {
      method: 'POST',
      body: formData.join('&')
    });

    // 3. Check for SAML Response
    const $saml = cheerio.load(loginResHtml);
    const samlAction = $saml('form').attr('action');
    if (samlAction && $saml('input[name="SAMLResponse"]').length > 0) {
      console.log("   SAML Response received, relaying back to Brone...");
      const samlData = [];
      $saml('input[type="hidden"]').each((i, el) => {
        samlData.push(`${$saml(el).attr('name')}=${encodeURIComponent($saml(el).attr('value'))}`);
      });
      curl(samlAction, {
        method: 'POST',
        body: samlData.join('&')
      });
    } else {
      console.log("   SAML Response NOT found. Body slice:");
      console.log(loginResHtml.substring(0, 1000));
    }

    // 4. Get Dashboard & Sesskey
    console.log(">> STEP 4: Fetching Dashboard...");
    const dashboardHtml = curl('https://brone.ub.ac.id/my/');
    const sesskeyMatch = dashboardHtml.match(/"sesskey":"([^"]+)"/);
    if (!sesskeyMatch) {
      console.log("   FAILED: Could not login or find sesskey.");
      return;
    }
    const sesskey = sesskeyMatch[1];
    console.log("   SUCCESS! Logged in. Sesskey:", sesskey);

    // 5. Fetch Tasks
    console.log(">> STEP 5: Fetching Tasks via AJAX...");
    const ajaxUrl = `https://brone.ub.ac.id/lib/ajax/service.php?sesskey=${sesskey}&info=core_calendar_get_action_events_by_timesort`;
    const payload = JSON.stringify([{
      index: 0,
      methodname: "core_calendar_get_action_events_by_timesort",
      args: {
        limitnum: 5,
        timesortfrom: Math.floor(Date.now() / 1000) - 86400,
        timesortto: Math.floor(Date.now() / 1000) + (86400 * 30)
      }
    }]);

    const cmdAjax = `curl.exe -s -k -b ${COOKIE_FILE} -X POST -d "${payload.replace(/"/g, '\\"')}" -H "Content-Type: application/json" "${ajaxUrl}"`;
    const ajaxRes = execSync(cmdAjax).toString();
    const data = JSON.parse(ajaxRes);
    const events = data[0].data.events;

    console.log("\n--- TASKS LIST ---");
    events.forEach((ev, i) => {
      console.log(`${i + 1}. ${ev.name}`);
      console.log(`   Course: ${ev.course.fullname}`);
      console.log(`   Deadline: ${new Date(ev.timesort * 1000).toLocaleString('id-ID')}`);
      console.log('-------------------');
    });

  } catch (err) {
    console.error("Test error:", err.message);
  } finally {
    if (fs.existsSync(COOKIE_FILE)) fs.unlinkSync(COOKIE_FILE);
  }
}

testSync();
