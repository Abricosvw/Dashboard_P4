import { useState } from 'react';
import { Settings, Grid3X3, Maximize, Minimize, Eye, EyeOff, Move } from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Switch } from '@/components/ui/switch';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Slider } from '@/components/ui/slider';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs';
import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogTrigger } from '@/components/ui/dialog';

export interface GaugeDisplaySettings {
  size: 'small' | 'medium' | 'large' | 'xlarge';
  columns: number;
  showTitles: boolean;
  showValues: boolean;
  showTargets: boolean;
  compactMode: boolean;
  gaugeStyle: 'circular' | 'arc' | 'linear';
  visibleGauges: {
    map: boolean;
    wastegate: boolean;
    tps: boolean;
    rpm: boolean;
    target: boolean;
    tcuStatus: boolean;
  };
  customPositions: boolean;
  positions: Record<string, { x: number; y: number; w: number; h: number }>;
}

interface GaugeSettingsProps {
  settings: GaugeDisplaySettings;
  onSettingsChange: (settings: GaugeDisplaySettings) => void;
}

export function GaugeSettings({ settings, onSettingsChange }: GaugeSettingsProps) {
  const [isOpen, setIsOpen] = useState(false);

  const updateSetting = <K extends keyof GaugeDisplaySettings>(
    key: K,
    value: GaugeDisplaySettings[K]
  ) => {
    onSettingsChange({ ...settings, [key]: value });
  };

  const updateVisibleGauge = (gaugeId: keyof GaugeDisplaySettings['visibleGauges'], visible: boolean) => {
    onSettingsChange({
      ...settings,
      visibleGauges: {
        ...settings.visibleGauges,
        [gaugeId]: visible
      }
    });
  };

  const getGaugeSize = () => {
    switch (settings.size) {
      case 'small': return { width: 150, height: 150 };
      case 'medium': return { width: 200, height: 200 };
      case 'large': return { width: 280, height: 280 };
      case 'xlarge': return { width: 350, height: 350 };
      default: return { width: 200, height: 200 };
    }
  };

  const visibleGaugeCount = Object.values(settings.visibleGauges).filter(Boolean).length;

  return (
    <Dialog open={isOpen} onOpenChange={setIsOpen}>
      <DialogTrigger asChild>
        <Button
          variant="outline"
          size="sm"
          className="bg-automotive-card hover:bg-gray-700 text-white border-gray-600"
        >
          <Settings className="w-4 h-4 mr-2" />
          Display Settings
        </Button>
      </DialogTrigger>
      
      <DialogContent className="max-w-4xl max-h-[80vh] overflow-y-auto bg-automotive-card border-gray-700 text-white">
        <DialogHeader>
          <DialogTitle className="font-orbitron text-automotive-accent">Gauge Display Settings</DialogTitle>
        </DialogHeader>

        <Tabs defaultValue="layout" className="w-full">
          <TabsList className="grid w-full grid-cols-4 bg-gray-800">
            <TabsTrigger value="layout" className="text-white">Layout</TabsTrigger>
            <TabsTrigger value="appearance" className="text-white">Appearance</TabsTrigger>
            <TabsTrigger value="visibility" className="text-white">Visibility</TabsTrigger>
            <TabsTrigger value="advanced" className="text-white">Advanced</TabsTrigger>
          </TabsList>

          <TabsContent value="layout" className="space-y-6">
            <Card className="border-gray-600 bg-gray-800">
              <CardHeader>
                <CardTitle className="text-white text-lg flex items-center">
                  <Grid3X3 className="w-5 h-5 mr-2 text-automotive-accent" />
                  Grid Layout
                </CardTitle>
              </CardHeader>
              <CardContent className="space-y-4">
                <div>
                  <Label className="text-gray-300">Columns per row</Label>
                  <div className="flex items-center space-x-4 mt-2">
                    <Slider
                      value={[settings.columns]}
                      onValueChange={([value]) => updateSetting('columns', value)}
                      min={1}
                      max={6}
                      step={1}
                      className="flex-1"
                    />
                    <span className="text-automotive-accent font-mono w-8">{settings.columns}</span>
                  </div>
                  <p className="text-sm text-gray-400 mt-1">
                    {visibleGaugeCount} visible gauges = {Math.ceil(visibleGaugeCount / settings.columns)} rows
                  </p>
                </div>

                <div>
                  <Label className="text-gray-300">Gauge Size</Label>
                  <Select value={settings.size} onValueChange={(value: any) => updateSetting('size', value)}>
                    <SelectTrigger className="mt-2 bg-gray-700 border-gray-600 text-white">
                      <SelectValue />
                    </SelectTrigger>
                    <SelectContent className="bg-gray-700 border-gray-600">
                      <SelectItem value="small">Small (150×150px)</SelectItem>
                      <SelectItem value="medium">Medium (200×200px)</SelectItem>
                      <SelectItem value="large">Large (280×280px)</SelectItem>
                      <SelectItem value="xlarge">Extra Large (350×350px)</SelectItem>
                    </SelectContent>
                  </Select>
                </div>

                <div className="flex items-center justify-between">
                  <Label className="text-gray-300">Custom Positioning</Label>
                  <Switch
                    checked={settings.customPositions}
                    onCheckedChange={(checked) => updateSetting('customPositions', checked)}
                  />
                </div>
                {settings.customPositions && (
                  <p className="text-sm text-automotive-warning">
                    <Move className="w-4 h-4 inline mr-1" />
                    Custom positioning allows drag & drop gauge arrangement
                  </p>
                )}
              </CardContent>
            </Card>
          </TabsContent>

          <TabsContent value="appearance" className="space-y-6">
            <Card className="border-gray-600 bg-gray-800">
              <CardHeader>
                <CardTitle className="text-white text-lg">Display Options</CardTitle>
              </CardHeader>
              <CardContent className="space-y-4">
                <div className="flex items-center justify-between">
                  <Label className="text-gray-300">Show Titles</Label>
                  <Switch
                    checked={settings.showTitles}
                    onCheckedChange={(checked) => updateSetting('showTitles', checked)}
                  />
                </div>

                <div className="flex items-center justify-between">
                  <Label className="text-gray-300">Show Values</Label>
                  <Switch
                    checked={settings.showValues}
                    onCheckedChange={(checked) => updateSetting('showValues', checked)}
                  />
                </div>

                <div className="flex items-center justify-between">
                  <Label className="text-gray-300">Show Target Indicators</Label>
                  <Switch
                    checked={settings.showTargets}
                    onCheckedChange={(checked) => updateSetting('showTargets', checked)}
                  />
                </div>

                <div className="flex items-center justify-between">
                  <Label className="text-gray-300">Compact Mode</Label>
                  <Switch
                    checked={settings.compactMode}
                    onCheckedChange={(checked) => updateSetting('compactMode', checked)}
                  />
                </div>

                <div>
                  <Label className="text-gray-300">Gauge Style</Label>
                  <Select value={settings.gaugeStyle} onValueChange={(value: any) => updateSetting('gaugeStyle', value)}>
                    <SelectTrigger className="mt-2 bg-gray-700 border-gray-600 text-white">
                      <SelectValue />
                    </SelectTrigger>
                    <SelectContent className="bg-gray-700 border-gray-600">
                      <SelectItem value="circular">Circular Arc</SelectItem>
                      <SelectItem value="arc">Half Arc</SelectItem>
                      <SelectItem value="linear">Linear Bar</SelectItem>
                    </SelectContent>
                  </Select>
                </div>
              </CardContent>
            </Card>
          </TabsContent>

          <TabsContent value="visibility" className="space-y-6">
            <Card className="border-gray-600 bg-gray-800">
              <CardHeader>
                <CardTitle className="text-white text-lg flex items-center">
                  <Eye className="w-5 h-5 mr-2 text-automotive-accent" />
                  Visible Gauges
                </CardTitle>
              </CardHeader>
              <CardContent className="space-y-4">
                <div className="grid grid-cols-2 gap-4">
                  <div className="flex items-center justify-between">
                    <Label className="text-gray-300">MAP Pressure</Label>
                    <Switch
                      checked={settings.visibleGauges.map}
                      onCheckedChange={(checked) => updateVisibleGauge('map', checked)}
                    />
                  </div>

                  <div className="flex items-center justify-between">
                    <Label className="text-gray-300">Wastegate Position</Label>
                    <Switch
                      checked={settings.visibleGauges.wastegate}
                      onCheckedChange={(checked) => updateVisibleGauge('wastegate', checked)}
                    />
                  </div>

                  <div className="flex items-center justify-between">
                    <Label className="text-gray-300">Throttle Position</Label>
                    <Switch
                      checked={settings.visibleGauges.tps}
                      onCheckedChange={(checked) => updateVisibleGauge('tps', checked)}
                    />
                  </div>

                  <div className="flex items-center justify-between">
                    <Label className="text-gray-300">Engine RPM</Label>
                    <Switch
                      checked={settings.visibleGauges.rpm}
                      onCheckedChange={(checked) => updateVisibleGauge('rpm', checked)}
                    />
                  </div>

                  <div className="flex items-center justify-between">
                    <Label className="text-gray-300">Target Boost</Label>
                    <Switch
                      checked={settings.visibleGauges.target}
                      onCheckedChange={(checked) => updateVisibleGauge('target', checked)}
                    />
                  </div>

                  <div className="flex items-center justify-between">
                    <Label className="text-gray-300">TCU Status</Label>
                    <Switch
                      checked={settings.visibleGauges.tcuStatus}
                      onCheckedChange={(checked) => updateVisibleGauge('tcuStatus', checked)}
                    />
                  </div>
                </div>

                <div className="mt-4 p-3 bg-gray-700 rounded">
                  <p className="text-sm text-gray-300">
                    Visible: <span className="text-automotive-accent font-mono">{visibleGaugeCount}</span> gauges
                  </p>
                  <p className="text-sm text-gray-400">
                    Layout: <span className="text-automotive-accent font-mono">{settings.columns}</span> columns × 
                    <span className="text-automotive-accent font-mono ml-1">{Math.ceil(visibleGaugeCount / settings.columns)}</span> rows
                  </p>
                </div>
              </CardContent>
            </Card>
          </TabsContent>

          <TabsContent value="advanced" className="space-y-6">
            <Card className="border-gray-600 bg-gray-800">
              <CardHeader>
                <CardTitle className="text-white text-lg">Advanced Settings</CardTitle>
              </CardHeader>
              <CardContent className="space-y-4">
                <div className="grid grid-cols-2 gap-4">
                  <Button
                    variant="outline"
                    className="bg-gray-700 border-gray-600 hover:bg-gray-600 text-white"
                    onClick={() => {
                      // Reset to default positions
                      updateSetting('positions', {});
                      updateSetting('customPositions', false);
                    }}
                  >
                    Reset Layout
                  </Button>

                  <Button
                    variant="outline"
                    className="bg-gray-700 border-gray-600 hover:bg-gray-600 text-white"
                    onClick={() => {
                      // Show all gauges
                      updateSetting('visibleGauges', {
                        map: true,
                        wastegate: true,
                        tps: true,
                        rpm: true,
                        target: true,
                        tcuStatus: true
                      });
                    }}
                  >
                    Show All Gauges
                  </Button>

                  <Button
                    variant="outline"
                    className="bg-gray-700 border-gray-600 hover:bg-gray-600 text-white"
                    onClick={() => {
                      // Fullscreen mode presets
                      updateSetting('size', 'xlarge');
                      updateSetting('columns', 2);
                      updateSetting('compactMode', false);
                    }}
                  >
                    Fullscreen Mode
                  </Button>

                  <Button
                    variant="outline"
                    className="bg-gray-700 border-gray-600 hover:bg-gray-600 text-white"
                    onClick={() => {
                      // Compact dashboard
                      updateSetting('size', 'small');
                      updateSetting('columns', 4);
                      updateSetting('compactMode', true);
                      updateSetting('showTitles', false);
                    }}
                  >
                    Compact Dashboard
                  </Button>
                </div>

                <div className="mt-4 p-3 bg-gray-700 rounded">
                  <h4 className="text-sm font-semibold text-automotive-accent mb-2">Quick Presets:</h4>
                  <ul className="text-sm text-gray-300 space-y-1">
                    <li>• Fullscreen: 2 columns, extra large gauges</li>
                    <li>• Compact: 4 columns, small gauges, minimal text</li>
                    <li>• Reset: Returns to default 3-column layout</li>
                  </ul>
                </div>
              </CardContent>
            </Card>
          </TabsContent>
        </Tabs>

        <div className="flex justify-end space-x-2 pt-4 border-t border-gray-600">
          <Button
            variant="outline"
            onClick={() => setIsOpen(false)}
            className="bg-gray-700 border-gray-600 hover:bg-gray-600 text-white"
          >
            Cancel
          </Button>
          <Button
            onClick={() => setIsOpen(false)}
            className="bg-automotive-accent hover:bg-blue-400 text-white"
          >
            Apply Settings
          </Button>
        </div>
      </DialogContent>
    </Dialog>
  );
}