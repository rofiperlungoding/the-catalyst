import axios from 'axios';
import * as cheerio from 'cheerio';
import { wrapper } from 'axios-cookiejar-support';
import { CookieJar } from 'tough-cookie';

const BRONE_USERNAME = "255150307111073";
const BRONE_PASSWORD = "Bu4tR4w0n!!";

async function testSync() {
  console.log("--- BRONE SYNC TEST (Native Fetch) ---");
  console.log("Starting login flow to brone.ub.ac.id...");

  try {
    // 1. Get initial login page
    console.log(">> STEP 1: Initial Landing...");
    const response = await fetch('https://brone.ub.ac.id/login/index.php', {
      headers: {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36'
      }
    });

    console.log("   Status:", response.status);
    if (response.status === 403) {
      console.log("   Still blocked by Cloudflare. Let's try matching curl exactly...");
      const curlRes = await fetch('https://brone.ub.ac.id/login/index.php', {
        headers: { 'User-Agent': 'curl/7.88.1' }
      });
      console.log("   Status with curl UA:", curlRes.status);
    }

    // 2. Trigger SSO Redirect
    console.log(">> STEP 2: Triggering SSO Redirect...");
    const ssoRedirect = await session.get('https://brone.ub.ac.id/auth/saml2/login.php?wants=https%3A%2F%2Fbrone.ub.ac.id%2Fmy%2F');
    console.log("   Redirected to:", ssoRedirect.request.res.responseUrl);

    let $ = cheerio.load(ssoRedirect.data);
    const formAction = $('form').attr('action');

    if (!formAction) {
      console.log("ERROR: Could not find SSO form action. Data received:");
      console.log(ssoRedirect.data.substring(0, 500));
      return;
    }

    // Prepare credentials
    const formData = new URLSearchParams();
    $('input[type="hidden"]').each((i, el) => {
      formData.append($(el).attr('name'), $(el).attr('value'));
    });
    formData.append('username', BRONE_USERNAME);
    formData.append('password', BRONE_PASSWORD);

    console.log("Submitting credentials to IAM UB SSO...");
    const loginResponse = await session.post(formAction, formData.toString(), {
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
    });

    // Handle SAML Response
    console.log("Checking for SAML Response...");
    const $saml = cheerio.load(loginResponse.data);
    const samlAction = $saml('form').attr('action');

    if (samlAction && $saml('input[name="SAMLResponse"]').length > 0) {
      console.log("SAML Response found, submitting back to Brone...");
      const samlData = new URLSearchParams();
      $saml('input[type="hidden"]').each((i, el) => {
        samlData.append($saml(el).attr('name'), $saml(el).attr('value'));
      });
      await session.post(samlAction, samlData.toString(), {
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' }
      });
    } else {
      console.log("Maybe already logged in or SSO failed.");
    }

    // Get dashboard
    console.log("Navigating back to Dashboard...");
    const dashboard = await session.get('https://brone.ub.ac.id/my/');
    const sesskeyMatch = dashboard.data.match(/"sesskey":"([^"]+)"/);
    if (!sesskeyMatch) {
      console.log("ERROR: Login failed. Could not find sesskey in dashboard.");
      return;
    }
    const sesskey = sesskeyMatch[1];
    console.log("SUCCESS! Session active. Sesskey:", sesskey);

    // Fetch Timeline
    const ajaxUrl = `https://brone.ub.ac.id/lib/ajax/service.php?sesskey=${sesskey}&info=core_calendar_get_action_events_by_timesort`;
    const payload = [{
      index: 0,
      methodname: "core_calendar_get_action_events_by_timesort",
      args: {
        limitnum: 5,
        timesortfrom: Math.floor(Date.now() / 1000) - 86400,
        timesortto: Math.floor(Date.now() / 1000) + (86400 * 30)
      }
    }];

    console.log("Fetching timeline data...");
    const eventsResponse = await session.post(ajaxUrl, payload);
    const events = eventsResponse.data[0].data.events;

    console.log("\n--- TASKS FOUND ---");
    events.forEach((ev, i) => {
      console.log(`${i + 1}. [${ev.course.fullname}]`);
      console.log(`   Task: ${ev.name}`);
      console.log(`   Deadline: ${new Date(ev.timesort * 1000).toLocaleString('id-ID')}`);
      console.log('-------------------');
    });

  } catch (err) {
    console.error("Test failed: ", err.message);
    if (err.response) {
      console.log("Status Code:", err.response.status);
      console.log("Headers:", JSON.stringify(err.response.headers, null, 2));
    }
  }
}

testSync();
