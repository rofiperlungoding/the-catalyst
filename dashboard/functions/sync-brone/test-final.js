import ical from 'node-ical';

const URL = "https://brone.ub.ac.id/calendar/export_execute.php?userid=47907&authtoken=72d9b83b379963a24be5fe4c9dead7d4a517639e&preset_what=all&preset_time=custom";

async function test() {
  console.log(">> Fetching iCal data...");
  try {
    const events = await ical.async.fromURL(URL);
    let count = 0;

    console.log("\n--- YOUR BRONE ASSIGNMENTS ---");
    for (let k in events) {
      const ev = events[k];
      if (ev.type === 'VEVENT') {
        count++;
        console.log(`${count}. ${ev.summary}`);
        console.log(`   Deadline: ${ev.start.toLocaleString('id-ID')}`);
        console.log('-------------------');
      }
    }
    console.log(`\nTotal: ${count} tasks found.`);
    if (count > 0) console.log("\nSUCCESS! Jalur iCal tembus Cloudflare! 🚀");
  } catch (e) {
    console.error("Test failed:", e.message);
  }
}

test();
