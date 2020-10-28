/*
 *   Copyright (c) 2020 Project CHIP Authors
 *   All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

package com.google.chip.chiptool.commissioner.thread;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.NonNull;
import com.google.chip.chiptool.commissioner.NetworkCredential;

public class ThreadNetworkCredential implements NetworkCredential, Parcelable {
  private final int channel;
  private final int panid;
  @NonNull private final byte[] xpanid;
  @NonNull private final byte[] meshPrefix;
  @NonNull private final byte[] masterKey;

  public ThreadNetworkCredential(int channel,
                                 int panid,
                                 @NonNull byte[] xpanid,
                                 @NonNull byte[] meshPrefix,
                                 @NonNull byte[] masterKey) {
    this.channel = channel;
    this.panid = panid;
    this.xpanid = xpanid;
    this.meshPrefix = meshPrefix;
    this.masterKey = masterKey;
  }

  private ThreadNetworkCredential(Parcel in) {
    this(in.readInt(), in.readInt(), in.createByteArray(), in.createByteArray(), in.createByteArray());
  }

  public ThreadNetworkCredential(byte[] encoded) {
    this(makeParcel(encoded));
  }

  public int getChannel() { return channel; }
  public int getPanid() { return panid; }
  @NonNull public byte[] getXpanid() { return xpanid; }
  @NonNull public byte[] getMeshPrefix() { return meshPrefix; }
  @NonNull public byte[] getMasterKey() { return masterKey; }

  public static final Creator<ThreadNetworkCredential> CREATOR =
      new Creator<ThreadNetworkCredential>() {
        @Override
        public ThreadNetworkCredential createFromParcel(Parcel in) {
          return new ThreadNetworkCredential(in);
        }

        @Override
        public ThreadNetworkCredential[] newArray(int size) {
          return new ThreadNetworkCredential[size];
        }
      };

  @Override
  public int describeContents() {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel parcel, int i) {
    parcel.writeInt(this.channel);
    parcel.writeInt(this.panid);
    parcel.writeByteArray(this.xpanid);
    parcel.writeByteArray(this.meshPrefix);
    parcel.writeByteArray(this.masterKey);
  }

  @Override
  public byte[] getEncoded() {
    Parcel parcel = Parcel.obtain();
    this.writeToParcel(parcel, 0);
    byte[] encoded = parcel.marshall();
    parcel.recycle();
    return encoded;
  }

  private static Parcel makeParcel(byte[] encoded) {
    Parcel parcel = Parcel.obtain();
    parcel.unmarshall(encoded, 0, encoded.length);
    parcel.setDataPosition(0);
    return parcel;
  }
}
