import { ArrowDownRight, ArrowUpRight, type LucideIcon } from "lucide-react";
import { cn } from "../lib/utils";
import { memo } from "react";

type StatCardProps = {
    title: string;
    value: string | number;
    unit: string;
    icon: LucideIcon;
    trend?: {
        value: number;
        isPositive: boolean;
    };
    className?: string;
    color?: "blue" | "green" | "orange" | "red" | "purple" | "default";
};

export const StatCard = memo(function StatCard({ title, value, unit, icon: Icon, trend, className, color = "default" }: StatCardProps) {

    const colorStyles = {
        default: "text-slate-900 dark:text-slate-100",
        blue: "text-blue-600 dark:text-blue-400",
        green: "text-emerald-600 dark:text-emerald-400",
        orange: "text-orange-600 dark:text-orange-400",
        red: "text-red-600 dark:text-red-400",
        purple: "text-purple-600 dark:text-purple-400",
    };

    const iconBgStyles = {
        default: "bg-slate-50 dark:bg-slate-800/50 text-slate-500 dark:text-slate-400",
        blue: "bg-blue-50 dark:bg-blue-900/30 text-blue-500 dark:text-blue-400",
        green: "bg-emerald-50 dark:bg-emerald-900/30 text-emerald-500 dark:text-emerald-400",
        orange: "bg-orange-50 dark:bg-orange-900/30 text-orange-500 dark:text-orange-400",
        red: "bg-red-50 dark:bg-red-900/30 text-red-500 dark:text-red-400",
        purple: "bg-purple-50 dark:bg-purple-900/30 text-purple-500 dark:text-purple-400",
    };

    const numValue = typeof value === 'number' ? value.toFixed(1) : value;

    return (
        <div
            className={cn(
                "rounded-2xl p-6 bg-white dark:bg-slate-900/50 shadow-sm border border-slate-100 dark:border-slate-800/80 transition-colors duration-500",
                "hover:shadow-md hover:-translate-y-0.5 ease-out",
                className
            )}
        >
            <div className="flex justify-between items-start mb-4">
                <div className={cn("p-2.5 rounded-xl transition-colors duration-500", iconBgStyles[color])}>
                    <Icon size={20} strokeWidth={2.5} />
                </div>
                {trend && (
                    <div className={cn(
                        "flex items-center gap-1 text-[11px] font-bold px-2.5 py-1 rounded-full transition-colors duration-500",
                        trend.isPositive ? "bg-red-50 dark:bg-red-900/30 text-red-600 dark:text-red-400" : "bg-emerald-50 dark:bg-emerald-900/30 text-emerald-600 dark:text-emerald-400"
                    )}>
                        {trend.isPositive ? <ArrowUpRight size={12} strokeWidth={3} /> : <ArrowDownRight size={12} strokeWidth={3} />}
                        {trend.value}%
                    </div>
                )}
            </div>

            <div>
                {/* Label — medium weight, clear */}
                <h3 className="text-sm font-semibold text-slate-400 dark:text-slate-500 tracking-wide mb-1.5 transition-colors">{title}</h3>
                {/* Value — extra bold, prominent */}
                <div className="flex items-baseline gap-1.5">
                    <span className={cn("text-[2rem] font-black tracking-tighter leading-none font-mono-nums transition-colors duration-500", colorStyles[color])}>
                        {numValue}
                    </span>
                    <span className="text-sm font-bold text-slate-400 dark:text-slate-500 transition-colors uppercase">{unit}</span>
                </div>
            </div>
        </div>
    );
});
