import { useEffect, useState, memo, useCallback } from 'react';
import { Calendar, Clock, BookOpen, AlertCircle, CheckCircle2 } from 'lucide-react';
import { formatDistanceToNow, isPast, parseISO } from 'date-fns';
import { cn } from '../lib/utils';
import { supabase } from '../lib/supabase';
import type { BroneTask } from '../types';

type AssignmentTrackerProps = {
  tasks: BroneTask[];
  isLoading: boolean;
  onTaskUpdate?: () => void;
};

export const AssignmentTracker = memo(function AssignmentTracker({ tasks, isLoading, onTaskUpdate }: AssignmentTrackerProps) {
  const [sortedTasks, setSortedTasks] = useState<BroneTask[]>([]);
  const [markingId, setMarkingId] = useState<string | null>(null);

  useEffect(() => {
    const filtered = tasks
      .filter(task => task.status === 'pending')
      .sort((a, b) => parseISO(a.deadline).getTime() - parseISO(b.deadline).getTime());
    setSortedTasks(filtered);
  }, [tasks]);

  const handleMarkAsDone = useCallback(async (taskId: string) => {
    setMarkingId(taskId);
    try {
      const { error } = await supabase
        .from('brone_tasks')
        .update({ status: 'completed' })
        .eq('id', taskId);

      if (error) throw error;
      if (onTaskUpdate) onTaskUpdate();
    } catch (e) {
      console.error("Failed to update task", e);
    } finally {
      setMarkingId(null);
    }
  }, [onTaskUpdate]);

  if (isLoading) {
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
    <div className="bg-white rounded-2xl p-6 border border-slate-100 shadow-sm">
      <div className="flex items-center justify-between mb-6">
        <div className="flex items-center gap-2.5">
          <div className="p-2 bg-blue-50 text-blue-600 rounded-xl">
            <Calendar size={20} strokeWidth={2.5} />
          </div>
          <h2 className="text-lg font-bold text-slate-900 tracking-tight">University Tasks</h2>
        </div>
        <span className="text-xs font-bold px-2.5 py-1 bg-slate-50 text-slate-500 rounded-full uppercase tracking-wider">
          {sortedTasks.length} Pending
        </span>
      </div>

      <div className="space-y-4">
        {sortedTasks.length === 0 ? (
          <div className="py-8 text-center bg-slate-50 rounded-xl border border-dashed border-slate-200">
            <p className="text-sm font-medium text-slate-400">All tasks completed! 🚀</p>
          </div>
        ) : (
          sortedTasks.map((task) => {
            const deadline = parseISO(task.deadline);
            const isUrgent = !isPast(deadline) && (deadline.getTime() - Date.now() < 86400000);

            return (
              <div
                key={task.id}
                className={cn(
                  "group flex items-center gap-4 p-4 rounded-xl border transition-all duration-200",
                  isUrgent ? "border-red-100 bg-red-50/30" : "border-slate-50 bg-slate-50/50 hover:border-slate-200 hover:bg-white hover:shadow-sm"
                )}
              >
                <div className={cn(
                  "flex-shrink-0 w-12 h-12 flex items-center justify-center rounded-xl",
                  isUrgent ? "bg-red-100 text-red-600" : "bg-white text-slate-400 shadow-sm border border-slate-100"
                )}>
                  {isUrgent ? <AlertCircle size={22} /> : <BookOpen size={22} />}
                </div>

                <div className="flex-1 min-w-0">
                  <div className="flex items-center gap-2 mb-0.5">
                    <span className="text-[10px] font-black uppercase tracking-widest text-slate-400">
                      {task.course_name.split(' - ')[0] || task.course_name}
                    </span>
                  </div>
                  <h3 className="text-sm font-bold text-slate-900 truncate tracking-tight">
                    {task.task_title}
                  </h3>
                  <div className="flex items-center gap-3 mt-1">
                    <div className={cn(
                      "flex items-center gap-1.5 text-xs font-bold",
                      isUrgent ? "text-red-500" : "text-slate-500"
                    )}>
                      <Clock size={12} />
                      {isPast(deadline) ? 'Expired' : formatDistanceToNow(deadline, { addSuffix: true })}
                    </div>
                  </div>
                </div>

                <button
                  onClick={() => handleMarkAsDone(task.id)}
                  disabled={markingId === task.id}
                  className={cn(
                    "p-2.5 rounded-xl transition-all duration-200",
                    markingId === task.id ? "bg-slate-100 animate-pulse text-slate-300" : "text-slate-200 hover:bg-emerald-50 hover:text-emerald-500"
                  )}
                  title="Mark as done"
                >
                  <CheckCircle2 size={20} strokeWidth={2.5} />
                </button>
              </div>
            );
          })
        )}
      </div>

      <div className="mt-6 pt-6 border-t border-slate-50 flex items-center justify-between text-[10px] font-bold uppercase tracking-widest text-slate-400">
        <div className="flex items-center gap-1">
          <span className="w-1.5 h-1.5 rounded-full bg-emerald-500"></span>
          <span>Sync Active</span>
        </div>
        <span>Brone Dashboard</span>
      </div>
    </div>
  );
});
