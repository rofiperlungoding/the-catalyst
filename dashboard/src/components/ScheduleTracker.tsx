import { useEffect, useState, memo } from 'react';
import { Calendar, Clock } from 'lucide-react';
import { cn } from '../lib/utils';
import { supabase } from '../lib/supabase';
import type { ClassSchedule } from '../types';

type ScheduleTrackerProps = {
  isLoading: boolean;
};

const DAY_NAMES = ['Minggu', 'Senin', 'Selasa', 'Rabu', 'Kamis', 'Jumat', 'Sabtu'];

export const ScheduleTracker = memo(function ScheduleTracker({ isLoading }: ScheduleTrackerProps) {
  const [todaySchedule, setTodaySchedule] = useState<ClassSchedule[]>([]);
  const [activeDay, setActiveDay] = useState<string>(DAY_NAMES[new Date().getDay()]);
  const [allSchedules, setAllSchedules] = useState<ClassSchedule[]>([]);
  const [internalLoading, setInternalLoading] = useState(true);

  useEffect(() => {
    async function fetchAllSchedules() {
      setInternalLoading(true);
      const { data } = await supabase
        .from('class_schedule')
        .select('*')
        .order('start_time', { ascending: true });

      if (data) {
        setAllSchedules(data);
      }
      setInternalLoading(false);
    }
    fetchAllSchedules();
  }, []);

  useEffect(() => {
    const filtered = allSchedules.filter(s => s.day_of_week === activeDay);
    setTodaySchedule(filtered);
  }, [allSchedules, activeDay]);

  if (isLoading || internalLoading) {
    return (
      <div className="bg-white rounded-2xl p-6 border border-slate-100 shadow-sm animate-pulse">
        <div className="h-6 w-48 bg-slate-100 rounded mb-6"></div>
        {[1, 2, 3].map(i => (
          <div key={i} className="flex gap-4 mb-4">
            <div className="w-12 h-12 bg-slate-50 rounded-xl"></div>
            <div className="flex-1 space-y-2">
              <div className="h-4 w-32 bg-slate-100 rounded"></div>
              <div className="h-3 w-48 bg-slate-50 rounded"></div>
            </div>
          </div>
        ))}
      </div>
    );
  }

  return (
    <div className="bg-white rounded-2xl p-6 border border-slate-100 shadow-sm flex flex-col h-full">
      <div className="flex items-center justify-between mb-6">
        <div className="flex items-center gap-2.5">
          <div className="p-2 bg-indigo-50 text-indigo-600 rounded-xl">
            <Calendar size={20} strokeWidth={2.5} />
          </div>
          <h2 className="text-lg font-bold text-slate-900 tracking-tight">Class Schedule</h2>
        </div>
      </div>

      {/* Day Selector */}
      <div className="flex gap-1 overflow-x-auto pb-4 mb-2 no-scrollbar">
        {DAY_NAMES.filter(d => d !== 'Minggu' && d !== 'Sabtu' || allSchedules.some(s => s.day_of_week === d)).map(day => (
          <button
            key={day}
            onClick={() => setActiveDay(day)}
            className={cn(
              "px-3 py-1.5 rounded-lg text-[10px] font-bold uppercase tracking-wider transition-all duration-200 whitespace-nowrap",
              activeDay === day
                ? "bg-indigo-600 text-white shadow-md shadow-indigo-100"
                : "bg-slate-50 text-slate-400 hover:bg-slate-100 hover:text-slate-600"
            )}
          >
            {day}
          </button>
        ))}
      </div>

      <div className="space-y-3 flex-1 overflow-y-auto pr-1">
        {todaySchedule.length === 0 ? (
          <div className="py-8 text-center bg-slate-50 rounded-xl border border-dashed border-slate-200">
            <p className="text-sm font-medium text-slate-400">No classes for {activeDay}! 😴</p>
          </div>
        ) : (
          todaySchedule.map((item) => (
            <div
              key={item.id}
              className="group flex items-center gap-4 p-4 rounded-xl border border-slate-50 bg-slate-50/50 hover:border-indigo-100 hover:bg-white hover:shadow-sm transition-all duration-200"
            >
              <div className="flex-shrink-0 w-12 h-12 flex flex-col items-center justify-center rounded-xl bg-white text-indigo-500 shadow-sm border border-slate-100 group-hover:border-indigo-50 transition-colors">
                <span className="text-[10px] font-black leading-none mb-1 opacity-50 uppercase">RM</span>
                <span className="text-xs font-black tracking-tight">{item.room}</span>
              </div>

              <div className="flex-1 min-w-0">
                <h3 className="text-sm font-bold text-slate-900 truncate tracking-tight group-hover:text-indigo-600 transition-colors">
                  {item.subject}
                </h3>
                <div className="flex items-center gap-3 mt-1.5">
                  <div className="flex items-center gap-1.5 text-[11px] font-bold text-slate-500">
                    <Clock size={12} className="text-slate-400" />
                    <span>{item.start_time.substring(0, 5)} - {item.end_time.substring(0, 5)}</span>
                  </div>
                </div>
              </div>

              <div className="h-2 w-2 rounded-full bg-indigo-200 group-hover:bg-indigo-500 transition-colors"></div>
            </div>
          ))
        )}
      </div>

      <div className="mt-6 pt-6 border-t border-slate-50 flex items-center justify-between text-[10px] font-bold uppercase tracking-widest text-slate-400">
        <div className="flex items-center gap-1">
          <span className="w-1.5 h-1.5 rounded-full bg-indigo-400"></span>
          <span>Academic Sync</span>
        </div>
        <span>{activeDay} Schedule</span>
      </div>
    </div>
  );
});
