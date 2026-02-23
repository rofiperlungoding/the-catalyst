const ical = require('node-ical');
const { createClient } = require('@supabase/supabase-js');

const BRONE_ICAL_URL = process.env.BRONE_ICAL_URL || "https://brone.ub.ac.id/calendar/export_execute.php?userid=47907&authtoken=72d9b83b379963a24be5fe4c9dead7d4a517639e&preset_what=all&preset_time=custom";
const SUPABASE_URL = process.env.SUPABASE_URL;
const SUPABASE_KEY = process.env.SUPABASE_SERVICE_ROLE_KEY;

const supabase = createClient(SUPABASE_URL, SUPABASE_KEY);

exports.handler = async (event, context) => {
  console.log("--- START SMART BRONE SYNC ---");

  try {
    const webEvents = await ical.async.fromURL(BRONE_ICAL_URL);
    const tasks = [];

    for (let k in webEvents) {
      const ev = webEvents[k];
      if (ev.type === 'VEVENT') {
        let summary = ev.summary || "Untitled Task";
        let courseName = "University Task";

        if (summary.includes('[') && summary.includes(']')) {
          const match = summary.match(/\[(.*?)\]/);
          if (match) {
            courseName = match[1];
            summary = summary.replace(` [${courseName}]`, '').trim();
          }
        }

        tasks.push({
          course_name: courseName,
          task_title: summary,
          deadline: ev.end ? ev.end.toISOString() : ev.start.toISOString(),
          updated_at: new Date().toISOString()
        });
      }
    }

    console.log(`Actually parsed ${tasks.length} tasks.`);

    if (tasks.length > 0) {
      // Calling our smart SQL function via RPC
      const { error } = await supabase.rpc('sync_brone_tasks', { task_list: tasks });
      if (error) throw error;
      console.log("Supabase Smart Sync successful.");
    }

    return {
      statusCode: 200,
      body: JSON.stringify({ message: "Sync successful", count: tasks.length })
    };

  } catch (err) {
    console.error("Sync failed:", err.message);
    return {
      statusCode: 500,
      body: JSON.stringify({ error: err.message })
    };
  }
};
