import React from "react";
import { AnalogClusterStatsProvider } from "./solanaClusterStats";

type Props = { children: React.ReactNode };
export function StatsProvider({ children }: Props) {
  return <AnalogClusterStatsProvider>{children}</AnalogClusterStatsProvider>;
}
