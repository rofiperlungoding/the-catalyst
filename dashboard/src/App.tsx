import { useEffect, useState, useRef, useCallback } from "react";
import { supabase } from "./lib/supabase";
import type { Device, SensorReading } from "./types";
import { StatCard } from "./components/StatCard";
import { SensorChart } from "./components/SensorChart";
import { ComfortGauge } from "./components/ComfortGauge";
import { ScheduleTracker } from "./components/ScheduleTracker";
import { SettingsModal, type AlertSettings } from "./components/SettingsModal";
import { Toast, type ToastData } from "./components/Toast";
import { Activity, Droplet, Thermometer, Wifi, RefreshCw, Smartphone, Settings, BarChart3, CloudRain, Flame } from "lucide-react";
import { formatDistanceToNow } from "date-fns";

function App() {
  const [readings, setReadings] = useState<SensorReading[]>([]);
  const [latestReading, setLatestReading] = useState<SensorReading | null>(null);
  const [device, setDevice] = useState<Device | null>(null);
  const [loading, setLoading] = useState(true);

  const [isSettingsOpen, setIsSettingsOpen] = useState(false);
  const [alertSettings, setAlertSettings] = useState<AlertSettings>(() => {
    const saved = localStorage.getItem("catalyst_settings");
    return saved ? JSON.parse(saved) : {
      tempMin: 18,
      tempMax: 30,
      humidMin: 30,
      humidMax: 70,
      notificationsEnabled: true
    };
  });
  const [toasts, setToasts] = useState<ToastData[]>([]);
  const lastAlertTimeRef = useRef<number>(0);
  const liveTimeoutRef = useRef<ReturnType<typeof setTimeout>>(undefined);

  useEffect(() => {
    localStorage.setItem("catalyst_settings", JSON.stringify(alertSettings));
  }, [alertSettings]);

  const checkAlerts = useCallback((reading: SensorReading) => {
    if (!alertSettings.notificationsEnabled) return;
    const now = Date.now();
    if (now - lastAlertTimeRef.current < 60000) return;

    let alertTriggered = false;
    if (reading.temperature > alertSettings.tempMax) {
      addToast("warning", "High Temp!", `${reading.temperature}°C is too hot.`);
      alertTriggered = true;
    } else if (reading.temperature < alertSettings.tempMin) {
      addToast("info", "Chilly!", `${reading.temperature}°C is quite cold.`);
      alertTriggered = true;
    }
    if (reading.humidity > alertSettings.humidMax) {
      addToast("warning", "High Humidity", `${reading.humidity}% moisture level.`);
      alertTriggered = true;
    }
    if (alertTriggered) lastAlertTimeRef.current = now;
  }, [alertSettings]);

  const addToast = (type: ToastData["type"], title: string, message?: string) => {
    const id = Math.random().toString(36).substring(7);
    setToasts((prev) => [...prev, { id, type, title, message }]);
  };

  const dismissToast = useCallback((id: string) => {
    setToasts((prev) => prev.filter((t) => t.id !== id));
  }, []);

  const fetchInitialData = useCallback(async () => {
    setLoading(true);

    // 1. Fetch Latest Device Heartbeat FIRST
    const { data: deviceData } = await supabase
      .from("devices")
      .select("*")
      .limit(1)
      .single();

    if (deviceData) {
      setDevice(deviceData);
      // Log for debugging: confirm actual last_seen vs local time
      const lastSeen = new Date(deviceData.last_seen_at).getTime();
      const now = Date.now();
      const diffMinutes = (now - lastSeen) / 60000;
      console.log(`[Authenticity Check] Device last seen ${diffMinutes.toFixed(2)}m ago`);
    }

    // 2. Fetch Sensor Readings
    const { data: readingData } = await supabase
      .from("sensor_readings")
      .select("*")
      .order("recorded_at", { ascending: false })
      .limit(30);

    if (readingData && readingData.length > 0) {
      setReadings(readingData);
      setLatestReading(readingData[0]);
    }

    setLoading(false);
  }, []);

  useEffect(() => {
    fetchInitialData();

    const readingSubscription = supabase
      .channel("sensor-updates")
      .on(
        "postgres_changes",
        { event: "INSERT", schema: "public", table: "sensor_readings" },
        (payload) => {
          const newReading = payload.new as SensorReading;
          setLatestReading(newReading);
          setReadings((prev) => [newReading, ...prev].slice(0, 30));
          checkAlerts(newReading);
        }
      )
      .subscribe();

    const deviceSubscription = supabase
      .channel("device-status")
      .on(
        "postgres_changes",
        { event: "UPDATE", schema: "public", table: "devices" },
        (payload) => {
          setDevice(payload.new as Device);
        }
      )
      .subscribe();

    const interval = setInterval(() => {
      setDevice(d => d ? { ...d } : null);
    }, 60000);

    return () => {
      readingSubscription.unsubscribe();
      deviceSubscription.unsubscribe();
      clearInterval(interval);
      if (liveTimeoutRef.current) clearTimeout(liveTimeoutRef.current);
    };
  }, [alertSettings, checkAlerts, fetchInitialData]);

  const prevReading = readings.length > 1 ? readings[1] : null;
  const tempTrend = prevReading && latestReading ?
    { value: Math.abs(((latestReading.temperature - prevReading.temperature) / prevReading.temperature) * 100).toFixed(1) as any, isPositive: latestReading.temperature > prevReading.temperature }
    : undefined;

  const humidTrend = prevReading && latestReading ?
    { value: Math.abs(((latestReading.humidity - prevReading.humidity) / prevReading.humidity) * 100).toFixed(1) as any, isPositive: latestReading.humidity > prevReading.humidity }
    : undefined;

  // Authentic Status Calculation (No Mocks)
  const isDeviceOnline = device?.last_seen_at && (Date.now() - new Date(device.last_seen_at).getTime() < 120000); // 2 minutes window

  useEffect(() => {
    const checkDark = () => {
      const h = new Date().getHours();
      const dark = h >= 18 || h < 6;
      if (dark) {
        document.documentElement.classList.add('dark');
      } else {
        document.documentElement.classList.remove('dark');
      }
    };
    checkDark();
    const intv = setInterval(checkDark, 60000); // Check every minute
    return () => clearInterval(intv);
  }, []);

  return (
    <div className="min-h-screen bg-[#F8F9FA] dark:bg-slate-950 text-slate-900 dark:text-slate-100 font-sans pb-20 transition-colors duration-700">

      {/* ─── Sticky Header ─── */}
      <div className="sticky top-0 z-50 bg-[#F8F9FA]/95 dark:bg-slate-950/95 backdrop-blur-sm border-b border-slate-200/60 dark:border-slate-800/60 transition-colors duration-700">
        <div className="max-w-7xl mx-auto px-6 py-4">
          <div className="flex flex-row items-center justify-between gap-6">
            <div>
              <div className="flex items-center gap-2 mb-1">
                <span className={`inline-block h-2 w-2 rounded-full transition-colors duration-300 ${isDeviceOnline ? 'bg-emerald-500 animate-pulse' : 'bg-slate-300 dark:bg-slate-700'}`}></span>
                <span className="text-[10px] font-bold tracking-[0.2em] text-slate-400 dark:text-slate-500 uppercase">
                  {isDeviceOnline ? (latestReading && (Date.now() - new Date(latestReading.recorded_at).getTime() < 60000) ? 'Streaming LIVE' : 'ONLINE') : 'OFFLINE'}
                </span>
              </div>
              <h1 className="text-2xl font-black tracking-tight text-slate-900 dark:text-white transition-colors duration-700">
                The Catalyst
              </h1>
              <p className="text-xs font-medium text-slate-400 mt-0.5">
                Environmental Intelligence Platform
              </p>
            </div>

            <div className="flex items-center gap-2">
              <button onClick={() => setIsSettingsOpen(true)} className="p-2.5 rounded-xl hover:bg-slate-100 dark:hover:bg-slate-800 text-slate-500 dark:text-slate-400 transition-all duration-200">
                <Settings size={18} />
              </button>
              <button onClick={fetchInitialData} className="p-2.5 rounded-xl hover:bg-slate-100 dark:hover:bg-slate-800 text-slate-500 dark:text-slate-400 transition-all duration-200">
                <RefreshCw size={18} className={loading ? "animate-spin" : ""} />
              </button>
            </div>
          </div>
        </div>
      </div>

      <div className="max-w-7xl mx-auto px-6 pt-8 space-y-8">

        {/* ─── Metric Cards ─── */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-5">
          <StatCard
            title="Temperature"
            value={latestReading?.temperature || 0}
            unit="°C"
            icon={Thermometer}
            color={latestReading && latestReading.temperature > alertSettings.tempMax ? "red" : "blue"}
            trend={tempTrend}
          />
          <StatCard
            title="Humidity"
            value={latestReading?.humidity || 0}
            unit="%"
            icon={CloudRain}
            color="blue"
            trend={humidTrend}
          />
          <StatCard
            title="Comfort Score"
            value={latestReading?.comfort_score || 0}
            unit="/ 100"
            icon={Activity}
            color="green"
          />

          {/* ─── Device Status Card ─── */}
          <div className="bg-white dark:bg-slate-900/50 rounded-2xl p-6 shadow-sm border border-slate-100 dark:border-slate-800/80 hover:shadow-md hover:-translate-y-0.5 transition-all duration-500 ease-out flex flex-col justify-between">
            <div className="flex justify-between items-start">
              <div className="flex items-center gap-3">
                <div className="p-2.5 rounded-full bg-purple-50 dark:bg-purple-900/30 text-purple-500 dark:text-purple-400 transition-colors">
                  <Wifi size={20} strokeWidth={2.5} />
                </div>
                <span className="text-sm font-semibold text-slate-500 dark:text-slate-400 tracking-wide transition-colors">Status</span>
              </div>
              {isDeviceOnline && (
                <span className="inline-block h-2.5 w-2.5 rounded-full bg-emerald-500 animate-pulse"></span>
              )}
            </div>
            <div className="mt-4">
              <div className="text-3xl font-black text-slate-900 dark:text-slate-100 tracking-tight leading-none transition-colors">
                {isDeviceOnline ? "Online" : "Offline"}
              </div>
              <div className="text-xs font-medium text-slate-400 mt-1.5 whitespace-nowrap overflow-hidden text-ellipsis">
                {device?.last_seen_at ?
                  `Last seen ${formatDistanceToNow(new Date(device.last_seen_at), { addSuffix: true })}`
                  : "Unknown"}
              </div>
            </div>
          </div>
        </div>

        {/* ─── Bento Grid ─── */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-5">

          {/* Temperature Chart */}
          <div className="lg:col-span-2 bg-white dark:bg-slate-900/50 rounded-2xl p-8 shadow-sm border border-slate-100 dark:border-slate-800/80 transition-colors duration-500">
            <div className="flex justify-between items-center mb-6">
              <div>
                <h3 className="text-lg font-black text-slate-900 dark:text-slate-100 tracking-tight transition-colors">Temperature Trend</h3>
                <p className="text-slate-400 text-sm font-medium mt-0.5">Real-time sensor readings</p>
              </div>
              <div className="bg-blue-50 dark:bg-blue-900/30 text-blue-500 dark:text-blue-400 p-2.5 rounded-xl transition-colors">
                <BarChart3 size={18} />
              </div>
            </div>
            <div className="h-[300px]">
              <SensorChart
                data={readings}
                dataKey="temperature"
                unit="°C"
                color="#3b82f6"
              />
            </div>
          </div>

          {/* Comfort Gauge */}
          <div className="bg-white dark:bg-slate-900/50 rounded-2xl shadow-sm border border-slate-100 dark:border-slate-800/80 flex flex-col items-center justify-center transition-colors duration-500">
            <ComfortGauge
              score={latestReading?.comfort_score ?? 0}
              label={(latestReading?.comfort_lvl ?? "--").replace("_", " ")}
            />
          </div>

          {/* Humidity Chart */}
          <div className="lg:col-span-2 bg-white dark:bg-slate-900/50 rounded-2xl p-8 shadow-sm border border-slate-100 dark:border-slate-800/80 transition-colors duration-500">
            <div className="flex justify-between items-center mb-6">
              <div>
                <h3 className="text-lg font-black text-slate-900 dark:text-slate-100 tracking-tight transition-colors">Humidity Analysis</h3>
                <p className="text-slate-400 text-sm font-medium mt-0.5">Ambient moisture levels</p>
              </div>
              <div className="bg-cyan-50 dark:bg-cyan-900/30 text-cyan-500 dark:text-cyan-400 p-2.5 rounded-xl transition-colors">
                <Droplet size={18} />
              </div>
            </div>
            <div className="h-[250px]">
              <SensorChart
                data={readings}
                dataKey="humidity"
                unit="%"
                color="#06b6d4"
              />
            </div>
          </div>

          {/* Device Details */}
          <div className="bg-white dark:bg-slate-900/50 rounded-2xl p-8 shadow-sm border border-slate-100 dark:border-slate-800/80 transition-colors duration-500">
            <h3 className="text-base font-black text-slate-900 dark:text-slate-100 mb-6 flex items-center gap-2.5 tracking-tight transition-colors">
              <Smartphone className="text-slate-400 dark:text-slate-500" size={18} /> Device Details
            </h3>

            <div className="space-y-0 text-sm">
              <div className="flex justify-between items-center py-3 border-b border-slate-50 dark:border-slate-800/50">
                <span className="font-medium text-slate-500 dark:text-slate-400">Name</span>
                <span className="font-bold text-slate-800 dark:text-slate-200">{device?.device_name}</span>
              </div>
              <div className="flex justify-between items-center py-3 border-b border-slate-50 dark:border-slate-800/50">
                <span className="font-medium text-slate-500 dark:text-slate-400">IP Address</span>
                <span className="font-mono text-xs font-bold text-slate-600 dark:text-slate-300 bg-slate-50 dark:bg-slate-800/50 px-2.5 py-1 rounded-lg font-mono-nums">{device?.ip_address}</span>
              </div>
              <div className="flex justify-between items-center py-3 border-b border-slate-50 dark:border-slate-800/50">
                <span className="font-medium text-slate-500 dark:text-slate-400">Last Reading</span>
                <span className="font-bold text-slate-800 dark:text-slate-200 text-xs">
                  {latestReading?.recorded_at ? formatDistanceToNow(new Date(latestReading.recorded_at), { addSuffix: true }) : "--"}
                </span>
              </div>

              <div className="pt-5 grid grid-cols-2 gap-5">
                <div className="flex items-start gap-2.5">
                  <Flame size={16} className="text-orange-400 dark:text-orange-500 mt-0.5" />
                  <div>
                    <span className="block text-[10px] font-bold text-slate-400 dark:text-slate-500 uppercase tracking-wider mb-1">Heat Index</span>
                    <span className="text-lg font-black text-slate-900 dark:text-slate-100 font-mono-nums">{latestReading?.heat_index?.toFixed(1) || "--"}°</span>
                  </div>
                </div>
                <div className="flex items-start gap-2.5">
                  <Droplet size={16} className="text-blue-400 dark:text-blue-500 mt-0.5" />
                  <div>
                    <span className="block text-[10px] font-bold text-slate-400 dark:text-slate-500 uppercase tracking-wider mb-1">Dew Point</span>
                    <span className="text-lg font-black text-slate-900 dark:text-slate-100 font-mono-nums">{latestReading?.dew_point?.toFixed(1) || "--"}°</span>
                  </div>
                </div>
              </div>
            </div>
          </div>

          {/* Schedule Tracker */}
          <div className="lg:col-span-1">
            <ScheduleTracker
              isLoading={loading}
            />
          </div>

        </div>

        {/* ─── Footer ─── */}
        <div className="pt-8 pb-4 text-center">
          <p className="text-xs text-slate-300 font-medium">
            The Catalyst — Environmental Intelligence Platform · Powered by SF Pro Display
          </p>
        </div>

        {/* Toasts */}
        <div className="fixed bottom-6 right-6 z-50 flex flex-col gap-3 pointer-events-none w-full max-w-sm">
          {toasts.map((toast) => (
            <div key={toast.id} className="pointer-events-auto">
              <Toast toast={toast} onDismiss={dismissToast} className="shadow-lg border-slate-200 bg-white" />
            </div>
          ))}
        </div>

        <SettingsModal
          isOpen={isSettingsOpen}
          onClose={() => setIsSettingsOpen(false)}
          onSave={setAlertSettings}
          currentSettings={alertSettings}
        />

      </div>
    </div>
  );
}

export default App;
