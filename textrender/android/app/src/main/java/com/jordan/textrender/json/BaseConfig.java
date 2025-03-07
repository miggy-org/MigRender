package com.jordan.textrender.json;

import android.content.Context;
import android.content.SharedPreferences;

import com.google.gson.Gson;
import com.jordan.textrender.MiggyConst;

import java.io.Serializable;
import java.lang.reflect.Type;

public abstract class BaseConfig implements Serializable {

    protected abstract String getPrefsKey();

    public static <T extends BaseConfig> T loadFromJson(String jsonConfig, T newInstance) {
        if (jsonConfig == null || jsonConfig.isEmpty()) {
            return newInstance;
        }
        return new Gson().fromJson(jsonConfig, (Type) newInstance.getClass());
    }

    public static <T extends BaseConfig> T loadFromPrefs(Context context, T newInstance) {
        SharedPreferences prefs = context.getSharedPreferences(MiggyConst.PREFS_KEY, Context.MODE_PRIVATE);
        return loadFromJson(prefs.getString(newInstance.getPrefsKey(), ""), newInstance);
    }

    public String saveToJson() {
        return new Gson().toJson(this);
    }

    public void saveToPrefs(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(MiggyConst.PREFS_KEY, Context.MODE_PRIVATE);
        prefs.edit().putString(getPrefsKey(), new Gson().toJson(this)).apply();
    }
}
