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
        default: "text-slate-900",
        blue: "text-blue-600",
        green: "text-emerald-600",
        orange: "text-orange-600",
        red: "text-red-600",
        purple: "text-purple-600",
    };

    const iconBgStyles = {
        default: "bg-slate-50 text-slate-500",
        blue: "bg-blue-50 text-blue-500",
        green: "bg-emerald-50 text-emerald-500",
        orange: "bg-orange-50 text-orange-500",
        red: "bg-red-50 text-red-500",
        purple: "bg-purple-50 text-purple-500",
    };

    const numValue = typeof value === 'number' ? value.toFixed(1) : value;

    return (
        <div
            className={cn(
                "rounded-2xl p-6 bg-white shadow-sm border border-slate-100",
                "hover:shadow-md hover:-translate-y-0.5 transition-all duration-200 ease-out",
                className
            )}
        >
            <div className="flex justify-between items-start mb-4">
                <div className={cn("p-2.5 rounded-xl", iconBgStyles[color])}>
                    <Icon size={20} strokeWidth={2.5} />
                </div>
                {trend && (
                    <div className={cn(
                        "flex items-center gap-1 text-[11px] font-bold px-2.5 py-1 rounded-full",
                        trend.isPositive ? "bg-red-50 text-red-600" : "bg-emerald-50 text-emerald-600"
                    )}>
                        {trend.isPositive ? <ArrowUpRight size={12} strokeWidth={3} /> : <ArrowDownRight size={12} strokeWidth={3} />}
                        {trend.value}%
                    </div>
                )}
            </div>

            <div>
                {/* Label — medium weight, clear */}
                <h3 className="text-sm font-semibold text-slate-400 tracking-wide mb-1.5">{title}</h3>
                {/* Value — extra bold, prominent */}
                <div className="flex items-baseline gap-1.5">
                    <span className={cn("text-[2rem] font-black tracking-tighter leading-none font-mono-nums", colorStyles[color])}>
                        {numValue}
                    </span>
                    <span className="text-sm font-bold text-slate-400 uppercase">{unit}</span>
                </div>
            </div>
        </div>
    );
});
